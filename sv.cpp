// ブラウザクライアントとの通信ができるサーバー

#include <iostream>
#include <thread>

// Header files related with WebRTC.
// WebRTC関連のヘッダ
#include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>
#include <rtc_base/thread.h>
#include <system_wrappers/include/field_trial.h>

// signaling via websocket
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


#include "mrs3.h"

struct Ice {
  std::string candidate;
  std::string sdp_mid;
  int sdp_mline_index;
};

class Connection {
 public:
  const std::string name;

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;

  std::function<void(const std::string &)> on_sdp;
  std::function<void()> on_accept_ice;
  std::function<void(const Ice &)> on_ice;
  std::function<void()> on_success;
  std::function<void(const std::string &)> on_message;

  // When the status of the DataChannel changes, determine if the connection is complete.
  // DataChannelのstateが変化したら、接続が完了したか確かめる。
  void on_state_change() {
    std::cout << "state: " << data_channel->state() << std::endl;
    if (data_channel->state() == webrtc::DataChannelInterface::kOpen && on_success) {
      on_success();
    }
  }

  // After the SDP is successfully created, it is set as a LocalDescription and displayed as a string to be passed to
  // the other party.
  // SDPの作成が成功したら、LocalDescriptionとして設定し、相手に渡す文字列として表示する。
  void on_success_csd(webrtc::SessionDescriptionInterface *desc) {
    peer_connection->SetLocalDescription(ssdo, desc);

    std::string sdp;
    desc->ToString(&sdp);
    on_sdp(sdp);
  }

  // Convert the got ICE.
  // 取得したICEを変換する。
  void on_ice_candidate(const webrtc::IceCandidateInterface *candidate) {
    Ice ice;
    candidate->ToString(&ice.candidate);
    ice.sdp_mid         = candidate->sdp_mid();
    ice.sdp_mline_index = candidate->sdp_mline_index();
    on_ice(ice);
  }

  class PCO : public webrtc::PeerConnectionObserver {
   private:
    Connection &parent;

   public:
    PCO(Connection &parent) : parent(parent) {
    }

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::SignalingChange(" << new_state << ")" << std::endl;
    };

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::AddStream" << std::endl;
    };

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::RemoveStream" << std::endl;
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::DataChannel(" << data_channel << ", " << parent.data_channel.get() << ")"
                << std::endl;
      // The request recipient gets a DataChannel instance in the onDataChannel event.
      // リクエスト受信側は、onDataChannelイベントでDataChannelインスタンスをもらう。
      parent.data_channel = data_channel;
      parent.data_channel->RegisterObserver(&parent.dco);
    };

    void OnRenegotiationNeeded() override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::RenegotiationNeeded" << std::endl;
    };

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceConnectionChange(" << new_state << ")" << std::endl;
    };

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceGatheringChange(" << new_state << ")" << std::endl;
    };

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "PeerConnectionObserver::IceCandidate" << std::endl;
      parent.on_ice_candidate(candidate);
    };
  };

  class DCO : public webrtc::DataChannelObserver {
   private:
    Connection &parent;

   public:
    DCO(Connection &parent) : parent(parent) {
    }

    void OnStateChange() override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "DataChannelObserver::StateChange" << std::endl;
      parent.on_state_change();
    };

    // Message receipt.
    // メッセージ受信。
    void OnMessage(const webrtc::DataBuffer &buffer) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "DataChannelObserver::Message" << std::endl;
      if (parent.on_message) {
        parent.on_message(std::string(buffer.data.data<char>(), buffer.data.size()));
      }
    };

    void OnBufferedAmountChange(uint64_t previous_amount) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "DataChannelObserver::BufferedAmountChange(" << previous_amount << ")" << std::endl;
    };
  };

  class CSDO : public webrtc::CreateSessionDescriptionObserver {
   private:
    Connection &parent;

   public:
    CSDO(Connection &parent) : parent(parent) {
    }

    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "CreateSessionDescriptionObserver::OnSuccess" << std::endl;
      parent.on_success_csd(desc);
    };

    void OnFailure(webrtc::RTCError error) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "CreateSessionDescriptionObserver::OnFailure" << std::endl
                << error.message() << std::endl;
    };
  };

  class SSDO : public webrtc::SetSessionDescriptionObserver {
   private:
    Connection &parent;

   public:
    SSDO(Connection &parent) : parent(parent) {
    }

    void OnSuccess() override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "SetSessionDescriptionObserver::OnSuccess" << std::endl;
      if (parent.on_accept_ice) {
        parent.on_accept_ice();
      }
    };

    void OnFailure(webrtc::RTCError error) override {
      std::cout << parent.name << ":" << std::this_thread::get_id() << ":"
                << "SetSessionDescriptionObserver::OnFailure" << std::endl
                << error.message() << std::endl;
    };
  };

  PCO pco;
  DCO dco;
  rtc::scoped_refptr<CSDO> csdo;
  rtc::scoped_refptr<SSDO> ssdo;

  Connection(const std::string &name_) :
      name(name_),
      pco(*this),
      dco(*this),
      csdo(new rtc::RefCountedObject<CSDO>(*this)),
      ssdo(new rtc::RefCountedObject<SSDO>(*this)) {
  }
};

class Wrapper {
 public:
  const std::string name;
  std::unique_ptr<rtc::Thread> network_thread;
  std::unique_ptr<rtc::Thread> worker_thread;
  std::unique_ptr<rtc::Thread> signaling_thread;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
  webrtc::PeerConnectionInterface::RTCConfiguration configuration;
  Connection connection;

  Wrapper(const std::string name_) : name(name_), connection(name_) {
  }

  void on_sdp(std::function<void(const std::string &)> f) {
    connection.on_sdp = f;
  }

  void on_accept_ice(std::function<void()> f) {
    connection.on_accept_ice = f;
  }

  void on_ice(std::function<void(const Ice &)> f) {
    connection.on_ice = f;
  }

  void on_success(std::function<void()> f) {
    connection.on_success = f;
  }

  void on_message(std::function<void(const std::string &)> f) {
    connection.on_message = f;
  }

  void init() {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "init Main thread" << std::endl;

    // Using Google's STUN server.
    // GoogleのSTUNサーバを利用。
    webrtc::PeerConnectionInterface::IceServer ice_server;
    ice_server.uri = "stun:stun.l.google.com:19302";
    configuration.servers.push_back(ice_server);

    network_thread = rtc::Thread::CreateWithSocketServer();
    network_thread->Start();
    worker_thread = rtc::Thread::Create();
    worker_thread->Start();
    signaling_thread = rtc::Thread::Create();
    signaling_thread->Start();
    webrtc::PeerConnectionFactoryDependencies dependencies;
    dependencies.network_thread   = network_thread.get();
    dependencies.worker_thread    = worker_thread.get();
    dependencies.signaling_thread = signaling_thread.get();
    peer_connection_factory       = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));

    if (peer_connection_factory.get() == nullptr) {
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Error on CreateModularPeerConnectionFactory." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  void create_offer_sdp() {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "create_offer_sdp" << std::endl;

    connection.peer_connection =
        peer_connection_factory->CreatePeerConnection(configuration, nullptr, nullptr, &connection.pco);

    webrtc::DataChannelInit config;

    // Configuring DataChannel.
    // DataChannelの設定。
    connection.data_channel = connection.peer_connection->CreateDataChannel("data_channel", &config);
    connection.data_channel->RegisterObserver(&connection.dco);

    if (connection.peer_connection.get() == nullptr) {
      peer_connection_factory = nullptr;
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Error on CreatePeerConnection." << std::endl;
      exit(EXIT_FAILURE);
    }
    connection.peer_connection->CreateOffer(connection.csdo, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }

  void create_answer_sdp(const std::string &parameter) {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "create_answer_sdp" << std::endl;

    connection.peer_connection =
        peer_connection_factory->CreatePeerConnection(configuration, nullptr, nullptr, &connection.pco);

    if (connection.peer_connection.get() == nullptr) {
      peer_connection_factory = nullptr;
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Error on CreatePeerConnection." << std::endl;
      exit(EXIT_FAILURE);
    }
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *session_description(
        webrtc::CreateSessionDescription("offer", parameter, &error));
    if (session_description == nullptr) {
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Error on CreateSessionDescription." << std::endl
                << error.line << std::endl
                << error.description << std::endl;
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Offer SDP:begin" << std::endl
                << parameter << std::endl
                << "Offer SDP:end" << std::endl;
      exit(EXIT_FAILURE);
    }
    connection.peer_connection->SetRemoteDescription(connection.ssdo, session_description);
    connection.peer_connection->CreateAnswer(connection.csdo, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }

  void push_reply_sdp(const std::string &parameter) {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "push_reply_sdp" << std::endl;

    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface *session_description(
        webrtc::CreateSessionDescription("answer", parameter, &error));
    if (session_description == nullptr) {
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Error on CreateSessionDescription." << std::endl
                << error.line << std::endl
                << error.description << std::endl;
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Answer SDP:begin" << std::endl
                << parameter << std::endl
                << "Answer SDP:end" << std::endl;
      exit(EXIT_FAILURE);
    }
    connection.peer_connection->SetRemoteDescription(connection.ssdo, session_description);
  }

  void push_ice(const Ice &ice_it) {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "push_ice" << std::endl;

    webrtc::SdpParseError err_sdp;
    webrtc::IceCandidateInterface *ice =
        CreateIceCandidate(ice_it.sdp_mid, ice_it.sdp_mline_index, ice_it.candidate, &err_sdp);
    if (!err_sdp.line.empty() && !err_sdp.description.empty()) {
      std::cout << name << ":" << std::this_thread::get_id() << ":"
                << "Error on CreateIceCandidate" << std::endl
                << err_sdp.line << std::endl
                << err_sdp.description << std::endl;
      exit(EXIT_FAILURE);
    }
    connection.peer_connection->AddIceCandidate(ice);
  }

  void send(const std::string &parameter) {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "send" << std::endl;

    webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(parameter.c_str(), parameter.size()), true);
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "Send(" << connection.data_channel->state() << ")" << std::endl;
    connection.data_channel->Send(buffer);
  }

  void quit() {
    std::cout << name << ":" << std::this_thread::get_id() << ":"
              << "quit" << std::endl;

    // Close with the thread running.
    // スレッドが起動した状態でCloseする。
    connection.peer_connection->Close();
    connection.peer_connection = nullptr;
    connection.data_channel    = nullptr;
    peer_connection_factory    = nullptr;

    network_thread->Stop();
    worker_thread->Stop();
    signaling_thread->Stop();
  }
};

/////////////////

class Client : public Wrapper {
public:
    MrsConnectionId wsconnid;
    Client(std::string wrname, MrsConnectionId c) : Wrapper(wrname), wsconnid(c) {
        init();
        on_accept_ice([&]() {
                          //                             std::lock_guard<std::mutex> lock(mtx);
                          //                             ice_flg1 = true;
                          //                             cond.notify_all();
                          std::cout << "on_accept_ice" << std::endl;
                      });

        //        std::list<Ice> ice_list;
        on_ice([&](const Ice &ice) {
                   //                       std::lock_guard<std::mutex> lock(mtx);
                   //         ice_list.push_back(ice);
                   //                       cond.notify_all();
                   std::cout << "on_ice candidate:" << ice.candidate <<  std::endl;

                   rapidjson::Document doc;
                   doc.SetObject();
                   doc.AddMember("type","candidate",doc.GetAllocator());
                   rapidjson::Value cand_value;
                   cand_value.SetString(rapidjson::StringRef(ice.candidate.c_str()));
                   rapidjson::Value sdp_mid_value;
                   sdp_mid_value.SetString(rapidjson::StringRef(ice.sdp_mid.c_str()));
                   rapidjson::Value payload;
                   payload.SetObject();
                   payload.AddMember("candidate",cand_value,doc.GetAllocator());
                   payload.AddMember("sdpMid",sdp_mid_value, doc.GetAllocator());
                   payload.AddMember("sdpMLineIndex",ice.sdp_mline_index, doc.GetAllocator());
                   doc.AddMember("payload",payload,doc.GetAllocator());
                   rapidjson::StringBuffer strbuf;
                   rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                   doc.Accept(writer);
                   std::string payloadstr = strbuf.GetString();
                   int wr=mrs_send_ws_raw(wsconnid,payloadstr.c_str(),payloadstr.size());
                   mrs_print("on_ice mrs_send_ws_raw ret:%d",wr);
                   
               });
    

        on_message([&](const std::string &message) {
                       //                          std::lock_guard<std::mutex> lock(mtx);
                       //    message1 = message;
                       //    cond.notify_all();
                       std::cout << "on_message:" + message << std::endl;
                       send(message);
                   });

        on_sdp([&](const std::string &sdp) {
                   std::cout << "on_sdp. sdp:" << sdp << std::endl;
                   rapidjson::Document doc;
                   doc.SetObject();
                   doc.AddMember("type","answer",doc.GetAllocator());
                   rapidjson::Value sdp_value;
                   sdp_value.SetString(rapidjson::StringRef(sdp.c_str()));
                   rapidjson::Value payload;
                   payload.SetObject();
                   payload.AddMember("type","answer",doc.GetAllocator());
                   payload.AddMember("sdp",sdp_value,doc.GetAllocator());
                   doc.AddMember("payload",payload,doc.GetAllocator());
                   rapidjson::StringBuffer strbuf;
                   rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
                   doc.Accept(writer);
                   std::string payloadstr = strbuf.GetString();
                   int wr=mrs_send_ws_raw(wsconnid,payloadstr.c_str(),payloadstr.size());
                   mrs_print("on_sdp_send_ws_raw ret:%d",wr);
                    //                      std::lock_guard<std::mutex> lock(mtx);
                    //                      offer_sdp = sdp;
                    //                      cond.notify_all();
                });

        on_success([&]() {
                       //                          std::lock_guard<std::mutex> lock(mtx);
                       std::cout << "on_success" << std::endl;
                       //                          success_flg1 = true;
                       //                          cond.notify_all();
                   });
        
    }
};


void on_read_ws_raw_cb( MrsConnectionId connid, const char *data, uint32_t data_len) {
    mrs_print("@@ on_read_ws_raw_cb connid:%llx len:%d", connid, data_len );
    //mrs_dumpbin(data,data_len);
    mrs_print("WS Raw Data: '%.*s'",data_len,data);
    rapidjson::Document doc;
    doc.Parse(data,data_len); // TODO: error check
    std::string type = doc["type"].GetString();

    if(type=="ping") {
        mrs_print("PPPPPPPPPPPPPPPPPPPPPPPPING from websocket");
        int32_t wr=mrs_send_ws_raw(connid,data,data_len);
        if(wr<=0) mrs_print("send_ws_raw error ret:%d",wr);
    } else if(type=="offer") {
        std::string offer_sdp = doc["payload"]["sdp"].GetString();
        mrs_print("Received offer, SDP:'%s'",offer_sdp.c_str());
        Client *cl = (Client*)mrs_connection_get_data(connid);
        cl->create_answer_sdp(offer_sdp);
        

    }
    

}

void on_disconnect_cb(MrsConnectionId connid) {
    mrs_print("@@ on_disconnect_cb conn:%llx",connid);
}
void on_accept_cb( MrsServerId svid, MrsConnectionId newconnid) {
    mrs_print("@@ on_accept_cb svid:%llx cl_connid:%llx",svid,newconnid);
    mrs_set_read_ws_raw_callback(newconnid, on_read_ws_raw_cb);
    mrs_set_disconnect_callback(newconnid, on_disconnect_cb);
    Client *cl = new Client("webrtc",newconnid);
    mrs_connection_set_data(newconnid,cl);
}

void on_transport_accept_cb( MrsServerId svid, MrsConnectionId newconnid) {
    mrs_print("@@ transport accept callback. svid:%llx cl_connid:%llx", svid, newconnid);
}


int main(int argc, char *argv[]) {
    webrtc::field_trial::InitFieldTrialsFromString("");
    rtc::InitializeSSL();
    
    

    // signaling server
    mrs_initialize();
    MrsContextId mrsctx = mrs_context_create(0);
    MrsServerId ws_sv = mrs_server_create(mrsctx,MrsConnectionType::WS,"0.0.0.0",23456,10);
    mrs_print("ws id:%llx port=23456",ws_sv);
    if(ws_sv<0) return 1;
    mrs_server_set_accept_callback(ws_sv,on_accept_cb);
    mrs_server_set_transport_accept_callback(ws_sv,on_transport_accept_cb);
    
    
    while(1) {
        usleep(100*1000);
        std::cerr << ".";
        mrs_update();
    }
    return 0;
}
