using System.Collections;
using UnityEngine;
using UnityEngine.Networking;
using TMPro; // Using TextMeshPro component
using XCharts.Runtime; // Using XChart for chart operations

// Data structure definitions to match the JSON format returned by the API
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
    // API URL
    [SerializeField]
    private string apiUrl = "https://uclapi.com/workspaces/sensors/summary?survey_ids=119&survey_filter=student&token=uclapi-0ed0e9b489b2d31-bec048527c1a578-82b2ccf36ee20ca-2883c2e949f470e";

    // TextMeshPro UI elements
    [SerializeField]
    private TextMeshProUGUI surveyNameText;
    [SerializeField]
    private TextMeshProUGUI sensorsOccupiedText;
    [SerializeField]
    private TextMeshProUGUI sensorsAbsentText;
    [SerializeField]
    private TextMeshProUGUI sensorsOtherText;

    // PieChart from XChart
    [SerializeField]
    private PieChart pieChart;

    void Awake()
    {
        // Ensure required components are assigned
        if (surveyNameText == null || sensorsOccupiedText == null || sensorsAbsentText == null || sensorsOtherText == null)
        {
            Debug.LogError("TextMeshProUGUI components are not assigned in the Inspector.");
        }

        if (pieChart == null)
        {
            Debug.LogError("PieChart component is not assigned in the Inspector.");
        }
    }

    void Start()
    {
        // Start coroutine to fetch and update data
        StartCoroutine(UpdateDashboard());
    }

    IEnumerator UpdateDashboard()
    {
        while (true) // Loop to refresh data continuously
        {
            // Make an API request
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
                    // Retrieve JSON data
                    string jsonData = webRequest.downloadHandler.text;
                    Debug.Log("API response: " + jsonData);

                    // Parse JSON data
                    SurveyData parsedData = JsonUtility.FromJson<SurveyData>(jsonData);

                    // Update UI elements with the parsed data
                    UpdateUI(parsedData);

                    // Update pie chart with the data
                    UpdatePieChart(parsedData);
                }
            }

            // Wait for 10 seconds before updating again
            yield return new WaitForSeconds(10f);
        }
    }

    void UpdateUI(SurveyData data)
    {
        // Check if data was successfully retrieved and contains valid survey data
        if (data.ok && data.surveys.Length > 0)
        {
            Survey survey = data.surveys[0]; // Only handle the first survey data

            // Update survey name
            surveyNameText.text = survey.name;

            // Count total seats
            int totalSeats = survey.sensors_occupied + survey.sensors_absent;

            // Update sensor status
            sensorsOccupiedText.text = $"Occupied: {survey.sensors_occupied}";
            sensorsAbsentText.text = $"Absent: {survey.sensors_absent}";
            sensorsOtherText.text = $"Total: {totalSeats}";

            // If there are multiple maps, handle them accordingly
            if (survey.maps.Length > 0)
            {
                Map map = survey.maps[0]; // Only handle the first map data
                Debug.Log($"Map Name: {map.name}, Occupied Sensors: {map.sensors_occupied}");
            }
        }
        else
        {
            Debug.LogWarning("No valid survey data retrieved or API request failed");
        }
    }

    void UpdatePieChart(SurveyData data)
    {
        // Clear existing data in PieChart
       // pieChart.ClearData();   

        // Check if data was successfully retrieved and contains valid survey data
        if (data.ok && data.surveys.Length > 0)
        {

            Debug.Log("test");
            Survey survey = data.surveys[0]; // Only handle the first survey data

            // Add data to the PieChart
            pieChart.UpdateData("serie0",0, survey.sensors_absent);
            pieChart.UpdateData("serie0",1, survey.sensors_occupied);
          


            // Refresh the chart to display the new data
            pieChart.RefreshChart();
        }
        else
        {
            Debug.LogWarning("No valid survey data retrieved or API request failed");
        }
    }
}
