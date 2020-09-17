using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;

public class Test : MonoBehaviour
{



    [DllImport("__Internal")]
    private static extern void Hello();
    [DllImport("__Internal")]
    private static extern void Init();
    [DllImport("__Internal")]
    private static extern void Connect();
    [DllImport("__Internal")]
    private static extern void Ping();

    [DllImport("__Internal")]
    private static extern void HelloString(string str);

    [DllImport("__Internal")]
    private static extern void PrintFloatArray(float[] array, int size);

    [DllImport("__Internal")]
    private static extern int AddNumbers(int x, int y);

    [DllImport("__Internal")]
    private static extern string StringReturnValueFunction();

    [DllImport("__Internal")]
    private static extern void BindWebGLTexture(int texture);

    // Start is called before the first frame update
    void Start()
    {
        Init();

//        Hello();
        
//        HelloString("This is a string.");
        
        float[] myArray = new float[10];
        PrintFloatArray(myArray, myArray.Length);
        
        int result = AddNumbers(5, 7);
        Debug.Log(result);
        
        Debug.Log(StringReturnValueFunction());
        
        var texture = new Texture2D(0, 0, TextureFormat.ARGB32, false);
        BindWebGLTexture(texture.GetNativeTextureID());
        Connect();
    }

    // Update is called once per frame
    void Update()
    {
        if(Input.GetKeyDown(KeyCode.P)) { 
            Ping();
        }
    }
}
