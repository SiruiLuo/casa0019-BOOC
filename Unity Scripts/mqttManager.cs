using UnityEngine;
using M2MqttUnity;
using uPLibrary.Networking.M2Mqtt.Messages;

public class MqttManager : M2MqttUnityClient
{
    public delegate void MessageReceivedHandler(string topic, string message);
    public event MessageReceivedHandler OnMessageReceived;

    private string latestMessage;

    protected override void OnConnected()
    {
        base.OnConnected();
        Debug.Log("MQTT Connected!");

        // Subscribe to a topic
        client.Subscribe(new string[] { "student/CASA0014/ucfnuoa/" }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE });
        Debug.Log("Subscribed to topic: student/CASA0014/ucfnuoa/");
    }

    protected override void DecodeMessage(string topic, byte[] message)
    {
        string receivedMessage = System.Text.Encoding.UTF8.GetString(message);
        Debug.Log($"Received message from topic {topic}: {receivedMessage}");

        // Store the latest message
        latestMessage = receivedMessage;

        // Trigger the message received event
        OnMessageReceived?.Invoke(topic, receivedMessage);
    }

    protected override void OnConnectionFailed(string errorMessage)
    {
        Debug.LogError($"MQTT connection failed: {errorMessage}");
    }

    protected override void OnDisconnected()
    {
        Debug.LogWarning("MQTT Disconnected");
    }

    public void PublishMessage(string topic, string message)
    {
        if (client != null && client.IsConnected)
        {
            client.Publish(topic, System.Text.Encoding.UTF8.GetBytes(message), MqttMsgBase.QOS_LEVEL_AT_LEAST_ONCE, false);
            Debug.Log($"Published message to topic {topic}: {message}");
        }
        else
        {
            Debug.LogError("Cannot publish, MQTT client is not connected.");
        }
    }

    public string GetLatestMessage()
    {
        return latestMessage;
    }
}