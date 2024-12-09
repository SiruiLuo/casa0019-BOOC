using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using System; // For DateTime
using System.Globalization; // Force the return of the date format in English

public class TimeUpdater : MonoBehaviour
{
    // TextMeshPro UI elements
    [SerializeField] private TextMeshProUGUI date; // Date
    [SerializeField] private TextMeshProUGUI day; // Day
    [SerializeField] private TextMeshProUGUI time; // Time

    void Start()
    {
        if (date == null || day == null || time == null)
        {
            Debug.LogError("TextMeshProUGUI components are not assigned in the Inspector.");
            return;
        }

        // Start updating time every second
        InvokeRepeating("UpdateTime", 0f, 1f);
    }

    void UpdateTime()
    {
        // Get current DateTime
        DateTime now = DateTime.Now;

        // Use English (UK) culture for formatting
        CultureInfo englishCulture = new CultureInfo("en-GB");

        // Format and update UI
        date.text = now.ToString("dd/MM/yyyy", englishCulture); // Format: Day/Month/Year
        day.text = now.ToString("dddd", englishCulture);       // Day of the week (e.g., Monday)
        time.text = now.ToString("HH:mm", englishCulture);      // Time in 24-hour format
    }
}
