using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;
using Newtonsoft.Json.Linq;
using XCharts.Runtime; // 使用 XChart 来处理图表操作

// 数据类定义
[System.Serializable]
public class TimeAverageData
{
    public string time;           // 时间点，例如 "09:00:00"
    public int sensorsOccupied;   // 该时间点的平均已使用传感器数
}

[System.Serializable]
public class AveragesData
{
    public List<TimeAverageData> timeDataList; // 封装后的时间点数据列表
}

public class LineChartUpdater : MonoBehaviour
{
    // API URL
    [SerializeField]
    private string apiUrl = "https://uclapi.com/workspaces/sensors/averages/time?days=7&survey_ids=111&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e";

    // LineChart from XChart
    [SerializeField]
    private LineChart lineChart;

    void Awake()
    {
        if (lineChart == null)
        {
            Debug.LogError("LineChart component is not assigned in the Inspector.");
        }
    }

    void Start()
    {
        StartCoroutine(UpdateLineChartData());
    }

    IEnumerator UpdateLineChartData()
    {
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
                    Debug.Log("Received JSON Data: " + jsonData);

                    JObject parsedData = JObject.Parse(jsonData);
                    var averages = parsedData["surveys"]?[0]?["averages"] as JObject;

                    if (averages == null)
                    {
                        Debug.LogWarning("No 'averages' field found in the response.");
                        continue;
                    }

                    // Parse averages into AveragesData
                    AveragesData averagesData = ParseAverages(averages);
                    Debug.Log($"Parsed {averagesData.timeDataList.Count} time points from averages data.");

                    // Update the LineChart
                    UpdateLineChart(averagesData);
                }
            }

            yield return new WaitForSeconds(10f);
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
            string timeKey = entry.Key; // 时间点
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
                    Debug.Log($"Added data point - Time: {timeData.time}, Occupied: {timeData.sensorsOccupied}");
                    dataIndex++;
                }
                else
                {
                    Debug.Log($"Skipped time point {timeData.time} (outside 9:00-17:00 or not on the hour).");
                }
            }
            else
            {
                Debug.LogWarning($"Invalid time format: {timeData.time}");
            }
        }

        var xAxis = lineChart.GetChartComponent<XAxis>();
        if (xAxis != null)
        {
            xAxis.data = timeLabels;
            Debug.Log($"Updated X-Axis labels with {timeLabels.Count} items.");
        }

        lineChart.RefreshChart();
        Debug.Log("LineChart refreshed with new data.");
    }
}
