using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;
using Newtonsoft.Json.Linq; // For JSON parsing
using XCharts.Runtime; // For XChart operations
using System;
using M2MqttUnity;
using uPLibrary.Networking.M2Mqtt.Messages; // For MQTT functionality

[System.Serializable]
public class TimeAverageData
{
    public string time;           // Time point, e.g., "09:00:00"
    public int sensorsOccupied;   // Average number of sensors occupied at this time
}

[System.Serializable]
public class AveragesData
{
    public List<TimeAverageData> timeDataList; // Encapsulated list of time point data
}

public class LineChartUpdater : MonoBehaviour
{
    [SerializeField]
    private string apiUrl; // API URL for data retrieval

    [SerializeField]
    private LineChart lineChart; // Reference to the LineChart component

    [SerializeField]
    private MqttManager mqttManager; // Reference to the MQTT Manager

    void Awake()
    {
        if (mqttManager != null)
        {
            mqttManager.OnMessageReceived += HandleMqttMessage; // Subscribe to MQTT messages
        }
        else
        {
            Debug.LogError("MQTT Manager is not assigned in the Inspector.");
        }

        if (lineChart == null)
        {
            Debug.LogError("LineChart component is not assigned in the Inspector.");
        }
    }



    IEnumerator UpdateLineChartData()
    {
        if (string.IsNullOrEmpty(apiUrl))
        {
            Debug.LogWarning("API URL is not set. Cannot fetch data.");
            yield break;
        }

        Debug.Log($"Fetching data from API: {apiUrl}");
        while (true) // Loop to refresh line chart data
        {
            using (UnityWebRequest webRequest = UnityWebRequest.Get(apiUrl))
            {
                webRequest.SetRequestHeader("Accept", "application/json");
                yield return webRequest.SendWebRequest();

                if (webRequest.result == UnityWebRequest.Result.ConnectionError || webRequest.result == UnityWebRequest.Result.ProtocolError)
                {
                    Debug.LogError("API request failed: " + webRequest.error);
                }
                else
                {
                    string jsonData = webRequest.downloadHandler.text;
                    Debug.Log($"API response: {jsonData}");

                    JObject parsedData = JObject.Parse(jsonData);
                    var averages = parsedData["surveys"]?[0]?["averages"] as JObject;

                    if (averages == null)
                    {
                        Debug.LogWarning("No 'averages' field found in the response.");
                        continue;
                    }

                    AveragesData averagesData = ParseAverages(averages);
                    Debug.Log($"Parsed {averagesData.timeDataList.Count} time points from API data.");

                    UpdateLineChart(averagesData);
                }
            }

            yield return new WaitForSeconds(30f); // Refresh every 30 seconds
        }
    }

    AveragesData ParseAverages(JObject averagesJson)
    {
        var averagesData = new AveragesData
        {
            timeDataList = new List<TimeAverageData>()
        };

        foreach (var entry in averagesJson)
        {
            string timeKey = entry.Key; // Time point
            var timeData = entry.Value;

            if (timeData != null && timeData["sensors_occupied"] != null)
            {
                int sensorsOccupied = timeData["sensors_occupied"].ToObject<int>();
                averagesData.timeDataList.Add(new TimeAverageData
                {
                    time = timeKey,
                    sensorsOccupied = sensorsOccupied
                });
            }
            else
            {
                Debug.LogWarning($"Skipping time key {timeKey} due to missing data.");
            }
        }

        return averagesData;
    }

    void UpdateLineChart(AveragesData averagesData)
    {
        lineChart.ClearData();

        var timeLabels = new List<string>();
        int dataIndex = 0;

        foreach (var timeData in averagesData.timeDataList)
        {
            if (System.DateTime.TryParse(timeData.time, out System.DateTime dateTime))
            {
                int hour = dateTime.Hour;
                if (hour >= 9 && hour <= 17 && dateTime.Minute == 0)
                {
                    lineChart.AddData("serie0", dataIndex, timeData.sensorsOccupied);
                    timeLabels.Add(dateTime.ToString("HH:mm"));
                    dataIndex++;
                }
            }
        }

        var xAxis = lineChart.GetChartComponent<XAxis>();
        if (xAxis != null)
        {
            xAxis.data = timeLabels;
        }

        lineChart.RefreshChart();
        Debug.Log("LineChart updated successfully.");
    }

    private void HandleMqttMessage(string topic, string message)
    {
    Debug.Log($"MQTT Message Received - Topic: {topic}, Message: {message}");

    if (topic.TrimEnd('/') == "student/CASA0014/ucfnuoa")
    {
        try
        {
            var parsedMessage = JObject.Parse(message);
            if (parsedMessage.TryGetValue("button", out JToken buttonToken) && buttonToken.Type == JTokenType.String)
            {
                var buttonValue = buttonToken.ToString();
                Debug.Log($"Button value parsed: {buttonValue}");

                switch (buttonValue)
                {
                    case "0":
                        apiUrl = "https://uclapi.com/workspaces/sensors/averages/time?days=7&survey_ids=119&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e";
                        break;
                    case "1":
                        apiUrl = "https://uclapi.com/workspaces/sensors/averages/time?days=7&survey_ids=111&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e";
                        break;
                    case "2":
                        apiUrl = "https://uclapi.com/workspaces/sensors/averages/time?days=7&survey_ids=116&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e";
                        break;
                    default:
                        Debug.LogWarning($"Unknown button value: {buttonValue}");
                        return;
                }

                Debug.Log($"API URL updated to: {apiUrl}");

                StopAllCoroutines(); // 停止之前的更新循环
                StartCoroutine(UpdateLineChartData()); // 开始新的更新循环
            }
            else
            {
                Debug.LogWarning("Button field is missing or not a string.");
            }
        }
        catch (Exception ex)
        {
            Debug.LogError($"Error parsing MQTT message: {ex.Message}");
        }
    }
    else
    {
        Debug.LogWarning($"Received message for unexpected topic: {topic}");
    }
    }

}
