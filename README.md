# Event-Driven Restaurant Management Simulation

## Overview
A robust, event-driven simulation in C++ that models the operational workflow of a complex restaurant environment. The system processes a high volume of concurrent events (orders, food preparation, deliveries, and cancellations) across multiple timesteps to calculate efficiency metrics, utilization rates, and operational bottlenecks.

## Technical Highlights
* **Core Language:** C++ (Object-Oriented Programming)
* **Custom Data Structures:** Implemented custom linked lists, standard queues, and priority queues (e.g., `BestFitTableQueue`, `CancellablePriQueue`) from scratch without relying on the C++ Standard Template Library (STL).
* **Memory Management:** Extensive use of dynamic memory allocation and pointer manipulation to ensure memory-safe state transitions across thousands of simulation timesteps.
* **Algorithmic Efficiency:** Optimized queue management for routing dynamic entities (Chefs, Scooters, Orders, and Tables) based on varying priority metrics (e.g., table seat counts, VIP status, overwait conditions).

## Features
* **Multi-Mode Execution:** Supports Interactive (step-by-step), Silent (bulk output generation), and Stress Test modes.
* **Dynamic Entity Routing:** Handles VIP promotions, order cancellations, and dynamic resource allocation.
* **Statistical Analysis Engine:** Auto-generates detailed operational metrics including resource utilization percentages and average wait/service times.

## Getting Started
1. Clone the repository.
2. Open `Restaurant_Simulation_Team23.sln` in Visual Studio.
3. Build the solution (Build -> Build Solution).
4. Run the executable and follow the console prompts. Test case files are located in the `Test_Cases` directory.