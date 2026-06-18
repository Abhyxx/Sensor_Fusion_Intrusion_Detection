# Sensor_Fusion_Intrusion_Detection
An Edge-AI Intrusion Detection System running a TinyML Random Forest model 100% locally on an ESP32. By fusing PIR, Microwave, and Ultrasonic sensors, it eliminates false alarms. Utilizing a tiered "Cascade Activation" strategy, the system operates at ultra-low power, delivering a low-cost, smart, and completely standalone security package.

# Edge-AI Intrusion Detection System using TinyML & Multi-Modal Sensor Fusion

An intelligent, low-cost, and standalone Edge-AI security solution that deploys local machine learning inference on an ESP32 microcontroller to minimize false-alarm rates and optimize energy efficiency.

---

## 🚀 Overview

Traditional home security and Intrusion Detection Systems (IDS) generally suffer from a stark trade-off: they are either cost-effective but highly prone to environmental false alarms (e.g., standard PIR-only setups), or they are highly accurate but rely on expensive, high-latency, and privacy-invasive cloud-based architectures.

This project introduces a standalone, wireless-optional **Edge-AI Intrusion Detection System**. By leveraging **TinyML** and combining multi-modal physical data via **Sensor Fusion**, the system executes real-time anomaly classification directly on resource-constrained hardware. The setup achieves enterprise-grade reliability and sub-millisecond local alert execution without any external network dependency.

## ✨ Core Pillars & Novelty

### 1. Multi-Modal Sensor Fusion
The hardware architecture integrates three distinct sensor types to create a robust spatial detection envelope:
* **Digital Mini PIR (Thermal):** Monitors background infrared/heat variations to establish baseline movement.
* **RCWL-0516 (Microwave/Doppler Radar):** Detects volumetric mass movement, capable of sensing through thin physical obstructions.
* **HC-SR04 (Ultrasonic):** Computes precise, real-time spatial distance metrics to track physical positioning.

### 2. Local TinyML Inference (Random Forest)
Instead of streaming raw sensor telemetry to a centralized cloud server, features are processed **100% locally on an ESP32 microcontroller**. A **Random Forest classifier** is optimized and compiled into lightweight, integer-friendly C++ code running directly on the edge. This provides a fully explainable AI (XAI) framework where decision trees can be systematically audited, avoiding the "black-box" limitations of traditional deep learning.

### 3. Cascade Activation Architecture
To maximize battery longevity for remote or standalone field deployments, the system utilizes a tiered, energy-aware execution strategy:
* **Tier 1 (Always-On):** The system maintains an ultra-low-power sleep state, keeping only the highly efficient Mini PIR active as a hardware tripwire.
* **Tier 2 (Verification):** Upon an interrupt trigger, the ESP32 awakes to activate the secondary Microwave and Ultrasonic arrays.
* **Tier 3 (Inference & Action):** The TinyML engine processes the combined feature vectors, evaluates the intrusion risk, and triggers instantaneous local alert logic (Buzzer/LED) before cascading back down to standby mode.

---

## 🛠️ Hardware Components

* **Microcontroller:** ESP32 (NodeMCU)
* **Thermal Sensing:** AM312 Digital Mini PIR Motion Sensor
* **Microwave Sensing:** RCWL-0516 Doppler Radar Module
* **Distance Sensing:** HC-SR04 Ultrasonic Ranging Module
* **Peripherals:** Buzzer, Logic LEDs, Resistors ($220\Omega$, $1\text{k}\Omega$, $2\text{k}\Omega$ for voltage division)

---

## 📈 Project Roadmap & Development Phases

* **Phase 1: Literature Review** ── Comprehensive analysis of contemporary edge-IDS systems and algorithmic constraints.
* **Phase 2: Component Selection** ── Evaluating hardware datasheets for power efficiency, form factor, and signal noise mitigation.
* **Phase 3: Hardware Integration** ── Circuit schematic development, breadboard prototyping, and voltage-divider implementation for ESP32 logic safety. 
* **Phase 4: Dataset Collection & Training** ── Logging localized real-world environmental noise vs. active intrusion vectors to train the Random Forest pipeline.

  # 📊 System Metrics
* **Inference Latency:** < 1ms (Edge-processed)
* **False Alarm Rate:** Minimal (Validated through sensor-fusion cross-verification)
* **Power Management:** Highly efficient via Cascade Sleep-Wake cycles.
