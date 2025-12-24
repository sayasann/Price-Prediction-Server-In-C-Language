# Price Prediction Server

Price Prediction Server is a multi-threaded C application that trains and serves a **Multiple Linear Regression (MLR)** model over a network connection.  
The system loads structured CSV datasets, preprocesses them in parallel, trains a regression model, and allows users to obtain predictions interactively via a socket-based interface.

## Project Overview

The core purpose of this project is to demonstrate how **operating system concepts**—especially **POSIX threads, synchronization, and shared memory management**—can be applied to a real-world data processing and prediction problem.

The server:
- Parses CSV datasets containing mixed **numeric** and **categorical** attributes  
- Performs **min–max normalization** on numeric data  
- Encodes categorical data into numeric representations  
- Trains a **multiple linear regression model** using parallel computation  
- Accepts user input over a network connection and returns predictions  

All computationally intensive stages are explicitly parallelized to highlight concurrency and thread coordination.

## Key Features

- **Multi-threaded preprocessing**
  - One thread per numeric attribute for normalization
  - One thread per categorical attribute for encoding

- **Parallel model training**
  - Coefficient calculations are distributed across multiple threads
  - Thread-safe aggregation using proper synchronization primitives

- **Interactive prediction server**
  - Listens on a TCP port
  - Accepts user input via `telnet`
  - Returns both normalized and real-scale predictions

- **Consistent data pipeline**
  - The same normalization and encoding rules are applied during training and prediction
  - Stored min/max values ensure correct reverse-normalization

## Supported Datasets

The server is designed to work with multiple CSV datasets that contain:
- Numeric attributes
- Categorical attributes
- A single numeric target variable

Each dataset is analyzed dynamically at runtime, allowing the same pipeline to be reused without hardcoding schema details.

## System Architecture

High-level workflow:

1. **Dataset Loading**
   - CSV files are parsed into memory-resident data structures
   - Column types (numeric / categorical) are detected automatically

2. **Preprocessing**
   - Numeric attributes are normalized using min–max scaling
   - Categorical attributes are encoded into numeric form
   - All preprocessing steps run concurrently

3. **Model Training**
   - A multiple linear regression model is trained on normalized data
   - Regression coefficients are computed in parallel
   - The final model is stored in normalized form

4. **Prediction**
   - The user provides raw input values
   - Inputs are normalized and encoded
   - The model produces a normalized prediction
   - The result is reverse-normalized to real scale and returned

## Technologies Used

- **C (GNU GCC)**
- **POSIX Threads (pthreads)**
- **TCP Sockets**
- **Linux environment**

## Usage

The application runs as a server process.  
Users connect via a terminal client (e.g. `telnet`) to interactively request predictions.

The server guides the user step-by-step:
- Dataset selection
- Attribute input
- Prediction output

## Design Focus

This project emphasizes:
- Correct use of **threads and synchronization**
- Avoidance of data races and deadlocks
- Clear separation between preprocessing, training, and prediction stages
- Predictable and reproducible numerical behavior



---

