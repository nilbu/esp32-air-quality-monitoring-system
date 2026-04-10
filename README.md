# ESP32 Air Quality Monitoring Platform

IoT air quality monitoring platform based on ESP32, PMS5003 and BME280, with Google Sheets logging, n8n automation, Telegram integration and PostgreSQL/pgvector conversational memory.

---

## Features

- PM1 / PM2.5 / PM10 measurement
- Temperature, humidity and pressure measurement
- Median filter and EMA smoothing
- Humidity correction for PM values
- Sensor duty cycle and cleaning cycle
- WiFi data transmission
- Google Sheets logging via Google Apps Script
- n8n workflow integration
- Telegram-based interaction
- PostgreSQL + pgvector memory for conversational context

---

## Hardware

- ESP32 DevKit
- PMS5003 particulate matter sensor
- BME280 environmental sensor
- Capacitors: 220µF + 100nF

---

## System Architecture

Sensors → ESP32 → WiFi → Google Apps Script → Google Sheets

Extended flow: Google Sheets → n8n → PostgreSQL + pgvector memory → Telegram

![Architecture](images/system_architecture.jpg)

---

## Prototype

Breadboard prototype of the air quality monitoring station:

![Prototype](images/prototype.jpg)

---

## Results

Example sensor records logged to Google Sheets:

![Google Sheets Data](images/google_sheets_data.png)

---

## Documentation

- [Air Quality Station Documentation](docs/air_quality_station_documentation.pdf)

---

## Setup

Additional setup files are available in the repository:

- [Database initialization](database/init.sql)
- [Example SQL queries](database/example_queries.sql)
- [Database setup guide](docs/database_setup.md)
- [n8n setup guide](docs/n8n_setup.md)
- [n8n workflow JSON](n8n/telegram-weather-workflow-sanitized.json)

---

## Integrations Preview

Example n8n workflow:

![n8n Workflow](images/n8n_workflow.png)

Example Telegram interaction:

![Telegram Example](images/telegram_example.jpg)


Example n8n workflow export used for Telegram interaction, voice transcription, OpenWeather enrichment and PostgreSQL/pgvector-based conversational memory:
- [telegram-workflow.json](n8n/telegram-workflow.json)

---

## Author

Tomek B
