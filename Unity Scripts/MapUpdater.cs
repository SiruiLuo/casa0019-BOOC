using UnityEngine;
using Newtonsoft.Json.Linq;

public class GameObjectUpdater : MonoBehaviour
{
    public MqttManager mqttManager;
    public GameObject map1;
    public GameObject map2;
    public GameObject map3;

    void Start()
    {
        mqttManager.OnMessageReceived += OnMessageReceived;
    }

    void OnMessageReceived(string topic, string message)
    {
        JObject json = JObject.Parse(message);
        string buttonValue = json["button"].ToString();

        UpdateGameObject(buttonValue);
    }

    void UpdateGameObject(string buttonValue)
    {
        map1.SetActive(buttonValue == "0");
        map2.SetActive(buttonValue == "1");
        map3.SetActive(buttonValue == "2");
    }

    void OnDestroy()
    {
        if (mqttManager != null)
        {
            mqttManager.OnMessageReceived -= OnMessageReceived;
        }
    }
}