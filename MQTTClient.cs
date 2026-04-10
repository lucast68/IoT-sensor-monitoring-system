// Unity MonoBehaviour that connects to MQTT broker,
// Receives temperature data, and displays it in real-time with a scrolling graph for visualization.

using System.Collections.Concurrent;
using System;
using UnityEngine;
using UnityEngine.UI;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;
using UnityEngine.Events; // For UnityAction

public class MQTTClient : MonoBehaviour
{   
    // MQTT connection configuration
    private MQTTClient mqttClient;
    private string brokerAddress = "test.mosquitto.org"; // Public broker address
    private int brokerPort = 1883; // Default MQTT port
    private string topic = "test/topic"; // Topic to subscribe to

    // UI display Elements for graph and temperature data
    public Text temperaturetext;
    public rawImage graphRawImage;
    private Texture2D graphTexture;

    // Temperature data buffer - stores rolling window of 100 readings
    private float[] temperatureData = new float[100];
    private int dataIndex = 0;

    // Thread-safe queue for MQTT messages received on network thread
    // Prevents race conditions when updating UI from main thread
    private ConcurrentQueue<float> temperatureQueue = new ConcurrentQueue<float>();

    // UnityAction callback for main thread UI updates
    private UnityAction<float> updateTemperatureUI;

    // Define a JSON data structure to match the expected JSON format of the incoming MQTT messages
    // Expected format: {"sensor:" "DS18B20", "temperature": 23.5}
    [System.Serializable]
    public class SensorData
    {
        public string sensor;
        public float temperature;
    }

    void Start()
    {   
        // Initializes the callback for thread-safe UI updates
        updateTemperatureUI = new UnityAction<float>(UpdateUI);

        // Configures MQTT client with broker connection details
        mqttClient = new MQTTClient(brokerAddress, brokerPort, false, null, null, MqttSslProtocols.None);

        // Registers callback for when MQTT messages arrive
        mqttClient.MqttMsgPublishReceived += handleMessageReceived;

        // Connect to the MQTT broker
        ConnectToBroker();

        // Initializes the graph texture with 310x110 pixel dimensions
        graphTexture = new Texture2D(350, 110);
        graphRawImage.texture = graphTexture;

        // Clear graph to black background
        ClearGraphTexture();

        // Resize the graph display widget (500x100 pixels on screen)
        graphRawImage.rectTransform.sizeDelta = new Vector2(500, 100); // Size of the graph display (Width x Height)
    }

    void Update()
    {   
        // Process all queued messages in the main thread to update the UI (Required by Unity)
        while (temperatureQueue.TryDequeue(out float temperature))
        {   
            updateTemperatureUI.Invoke(temperature);
        }
    }

    void UpdateUI(float temperature)
    {   
        // Updates the temperature display with 2 decimal precision
        temperaturetext.text = "Temperature: " + temperature.ToString("F2") + " °C";

        // Adds new data point to the rolling window buffer
        if (dataPointIndex < temperatureData.Length)
        {
            temperatureData[dataPointIndex] = temperature;
            dataPointIndex++;
        }
        else
        {   
            // If buffer is full, shift all data to left and add new value at the end
            // (creates a scrolling effect showing last 100 readings)
            for (int i = 0; i < temperatureData.Length - 1; i++)
            {
                temperatureData[i] = temperatureData[i + 1];
            }
            temperatureData[temperatureData.Length - 1] = temperature;
        }

        // Redraws graph with the updated temperature data
        UpdateGraphTexture();
    }

    void ClearGraphTexture()
    {   
        // Initalizes graph with black background
        for (int y = 0; y < graphTexture.width; y++)
        {
            for (int x = 0; x < graphTexture.height; x++)
            {
                graphTexture.SetPixel(x, y, Color.black);
            }
        }
        graphTexture.Apply();
    }

    void UpdateGraphTexture()
    {   
        // Renders graph by mapping temperature values to Y-axis pixels
        for (int i = 0; i < temperatureData.Length - 1; i++)
        {   
            // Maps temperature values to vertical pixel positions (assuming temperature range 0-50°C)
            // Lerp interpolates between 0 and texture height based on temperature / 50
            int yPos = Mathf.RoundToInt (Mathf.Lerp(0, graphTexture.Height - 1, temperatureData[i] / 50));
            graphTexture.SetPixel (i, yPos, Color.red); // Plot as red pixel
        }
        // Applies the changes to the texture
        graphTexture.Apply();
    }

    // Connects to MQTT broker and subscribe to temperature topic
     void ConnectToBroker()
    {
        try
        {   
            // Connects with unique client ID to avoid conflicts
            mqttClient.Connect(Guid.NewGuid().ToString());

            // Subscribes to the desired MQTT topic at QoS level 0 (at most once delivery)
            mqttClient.Subscribe(new string[] { topic }, new byte[] { MqttMsgBase.QOS_LEVEL_AT_MOST_ONCE });
            Debug.Log("Connected to MQTT broker and subscribed to topic: " + topic);
        }
        catch (System.Exception ex)
        {
            Debug.LogError("Error connecting to MQTT broker: " + ex.Message);
        }
    }

    // Callback handler for when MQTT messages are received from the broker
    private void HandleMessageReceived(object sender, MqttMsgPublishEventArgs e)
    {   
        // Decodes messages payload from bytes to UTF-8 string
        string payload = System.Text.Encoding.UTF8.GetString(e.Message);
        Debug.Log("Message received: " + payload);

        // Parses JSON payload into SensorData object
        SensorData = SensorData = JsonUtility.FromJson<SensorData>(payload);

        // Processes temperature readings from DS18B20
        if (SensorData.sensor == "DS18B20")
        {   
            // Adds temperature to thread-safe queue for main thread processing
            temperatureQueue.Enqueue(SensorData.temperature);
        }
    }
    
    // Cleans up applicaiton exit to prevent connection leaks
    private void OnApplicationQuit()
    {   
        // Disconnects from the MQTT broker when the game closes
        if (mqttClient.IsConnected)
        {
            mqttClient.Disconnect();
        }
    }
}
