using System.Collections;
using UnityEngine;
using UnityEngine.Networking;
using TMPro; // Using TextMeshPro component
using XCharts.Runtime; // Using XChart for chart operations
using Newtonsoft.Json.Linq; // For JSON parsing
using M2MqttUnity; // Using MQTT for Unity
using uPLibrary.Networking.M2Mqtt.Messages; // MQTT message handling

[System.Serializable]
public class Map
{
    public int id;
    public string name;
    public int sensors_absent;
    public int sensors_occupied;
}

[System.Serializable]
public class Survey
{
    public int id;
    public string name;
    public Map[] maps;
    public int staff_survey;
    public int sensors_absent;
    public int sensors_occupied;
}

[System.Serializable]
public class SurveyData
{
    public bool ok;
    public Survey[] surveys;
}

public class Summarize : MonoBehaviour
{
    [SerializeField]
    private string apiUrl; // API URL for data retrieval

    [SerializeField]
    private TextMeshProUGUI surveyNameText;
    [SerializeField]
    private TextMeshProUGUI sensorsOccupiedText;
    [SerializeField]
    private TextMeshProUGUI sensorsAbsentText;
    [SerializeField]
    private TextMeshProUGUI sensorsOtherText;

    [SerializeField]
    private PieChart pieChart; // PieChart from XChart

    [SerializeField]
    private MqttManager mqttManager; // MQTT Manager reference

    private readonly string[] apiUrls = {
        "https://uclapi.com/workspaces/sensors/summary?survey_ids=119&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e",
        "https://uclapi.com/workspaces/sensors/summary?survey_ids=111&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e",
        "https://uclapi.com/workspaces/sensors/summary?survey_ids=116&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e"
    };

    private Coroutine updateCoroutine;

    void Awake()
    {
        // Ensure all UI components and PieChart are assigned
        if (surveyNameText == null || sensorsOccupiedText == null || sensorsAbsentText == null || sensorsOtherText == null)
        {
            Debug.LogError("TextMeshProUGUI components are not assigned in the Inspector.");
        }

        if (pieChart == null)
        {
            Debug.LogError("PieChart component is not assigned in the Inspector.");
        }

        if (mqttManager != null)
        {
            mqttManager.OnMessageReceived += HandleMqttMessage; // Subscribe to MQTT messages
        }
        else
        {
            Debug.LogError("MQTT Manager is not assigned in the Inspector.");
        }
    }

    void Start()
    {
        if (!string.IsNullOrEmpty(apiUrl))
        {
            StartCoroutineSafely();
        }
        else
        {
            Debug.LogWarning("API URL is empty. Waiting for MQTT message to set the URL.");
        }
    }

    private void StartCoroutineSafely()
    {
        if (updateCoroutine != null)
        {
            StopCoroutine(updateCoroutine);
        }
        updateCoroutine = StartCoroutine(UpdatePieChartData());
    }

    private IEnumerator UpdatePieChartData()
    {
        if (string.IsNullOrEmpty(apiUrl))
        {
            Debug.LogWarning("API URL is not set. Cannot fetch data.");
            yield break;
        }

        Debug.Log($"Fetching data from API: {apiUrl}");
        while (true)
        {
            using (UnityWebRequest webRequest = UnityWebRequest.Get(apiUrl))
            {
                webRequest.SetRequestHeader("Accept", "application/json");
                yield return webRequest.SendWebRequest();

                if (webRequest.result != UnityWebRequest.Result.Success)
                {
                    Debug.LogError($"API request failed: {webRequest.error}");
                }
                else
                {
                    string jsonData = webRequest.downloadHandler.text;
                    Debug.Log($"API response: {jsonData}");

                    SurveyData parsedData = JsonUtility.FromJson<SurveyData>(jsonData);
                    UpdateUI(parsedData);
                    UpdatePieChart(parsedData);
                }
            }

            yield return new WaitForSeconds(30f); // Refresh every 30 seconds
        }
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
                            apiUrl = apiUrls[0];
                            break;
                        case "1":
                            apiUrl = apiUrls[1];
                            break;
                        case "2":
                            apiUrl = apiUrls[2];
                            break;
                        default:
                            Debug.LogWarning($"Unknown button value: {buttonValue}");
                            return;
                    }

                    Debug.Log($"API URL updated to: {apiUrl}");
                    StartCoroutineSafely(); // Restart the coroutine with the new API URL
                }
                else
                {
                    Debug.LogWarning("Button field is missing or not a string.");
                }
            }
            catch (System.Exception ex)
            {
                Debug.LogError($"Error parsing MQTT message: {ex.Message}");
            }
        }
        else
        {
            Debug.LogWarning($"Received message for unexpected topic: {topic}");
        }
    }

    private void UpdateUI(SurveyData data)
    {
        if (data.ok && data.surveys.Length > 0)
        {
            Survey survey = data.surveys[0];

            surveyNameText.text = survey.name;
            sensorsOccupiedText.text = $"Occupied: {survey.sensors_occupied}";
            sensorsAbsentText.text = $"Absent: {survey.sensors_absent}";
            sensorsOtherText.text = $"Total: {survey.sensors_occupied + survey.sensors_absent}";
        }
        else
        {
            Debug.LogWarning("No valid survey data retrieved or API request failed");
        }
    }

    private void UpdatePieChart(SurveyData data)
    {
        if (data.ok && data.surveys.Length > 0)
        {
            Survey survey = data.surveys[0];

            pieChart.UpdateData("serie0", 0, survey.sensors_absent);
            pieChart.UpdateData("serie0", 1, survey.sensors_occupied);

            pieChart.RefreshChart();
        }
        else
        {
            Debug.LogWarning("No valid survey data retrieved or API request failed");
        }
    }
}
