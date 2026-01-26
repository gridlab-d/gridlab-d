# ibr blackboxmodels

As part of the DistribuDyn project funded by the U.S. Department of Energy Solar Energy Technology Office (SETO), advanced inverter-based-resource models were created to represent devices being deployed on the modern power system. The model in **inverter_dyn** represents a generalized inverter. This page represents blackbox models generalized from hardware tests and measurements of commercial hardware. The ``ibr_blackbox`` model is produced by the Oak Ridge National Laboratory as part of the DistribuDyn project. 

## Introduction

Inverter-based resource (IBR) models are necessary to analyze modern power system stability and create effective control strategies. Modeling IBRs in converter-rich power systems is crucial yet challenging due to the lack of commercial information on converter topologies and control parameters. This guide provides a step-by-step process for developing a black-box model of inverter-based resources (IBRs) or distributed energy resources (DERs) using a Convolutional Neural Network (CNN). The goal is to learn the input-output behavior of an IBR/DER without needing detailed physics-based equations, addressing adaptability and proprietary concerns without requiring internal system physics knowledge. This document will also provide guides on the integration and implementation in the open-source power distribution simulation and analysis tool, GridLAB-D™. 

## Grid-Following Inveter

A Grid-Following Inverter relies on Phase-Locked Loop (PLL) technology to track the phase and frequency of the grid voltage. Based on the measured grid parameters, it generates an output current waveform that is synchronized with the grid voltage. The inverter does not have the capability to regulate grid voltage or frequency; instead, it adjusts its operation to follow the grid’s existing conditions. The details of the grid-following inverter can be found here [[[1]](https://GridLAB-D™.shoutwiki.com/wiki/Spec:inverter_dyn#google_vignette)]. 

## CNN-Based Black-Box Modeling Framework

Traditional white-box models, which depend on detailed physics-based equations, can be both complex and computationally demanding. In contrast, black-box modeling with deep learning—particularly CNNs—provides an efficient alternative by learning the nonlinear behavior of IBRs directly from input-output data. Unlike white-box models, black-box approaches do not require explicit knowledge of internal system dynamics. Instead, they leverage measured data to capture system behavior, making them especially valuable for IBRs, where control algorithms and internal configurations are often proprietary or unavailable. 

### Data Pre-Processing

Before training a CNN model, we need a high-quality dataset containing input-output relationships of the IBR. In black-box modeling of IBRs, the inputs and outputs are defined based on measurable electrical quantities. For a three-phase IBR, the input is the three-phase voltage at the point of common coupling (PCC), while the output is the three-phase current injected into the grid. Similarly, for a single-phase IBR, the input is the single-phase voltage at the PCC, and the output is the single-phase current supplied to the grid. These input-output relationships form the basis for training deep learning models, such as CNNs, to accurately capture and predict the dynamic behavior of IBRs. For the training and testing data, it can be collected from both single-phase and three-phase systems testbed under various transient conditions. These conditions may include DC input changes, voltage ride-through, and frequency ride-through events, which impact the dynamic behavior of the inverter. To ensure high-quality input for the CNN-based black-box model, several preprocessing steps are applied to the collected data: 

#### Remove Startup Data

Eliminating startup transients helps focus on the steady-state and dynamic behavior of the IBR under normal and transient conditions. 

#### Downsample the Data

Reducing the data points ensures a uniform time step across all signals while preserving essential patterns. This optimizes computational efficiency without compromising the model’s ability to capture system dynamics. 

#### Normalize the Data

Scaling techniques such as min-max normalization or standardization are applied to ensure consistent feature ranges, improving model convergence and accuracy. 

#### Reshape Data into a Tensor Structure

Since time-series data requires structured input for deep learning models: 

  * Zero Padding: Zeros are added at the beginning of the dataset to compensate for missing initial observations.
  * Sequence Formatting: The input data (x_data) and output data (y_data) are structured into sequential windows for time-series prediction tasks.
The pseudocode for the data preprocessing is given as below example. 
    
    
        def reshape_data(data, window_size):
          x_data = []  # Input data (sequences of past observations)
          y_data = []  # Output data (future observations or averages)
          # Add zeros at the beginning for the initial conditions
          data_with_zeros = prepend_zeros(data, window_size)
          # Reshape input data
          x_data = create_input_windows(data_with_zeros, window_size)
          # Reshape output data and take the average for each window
          y_data = create_output_windows(data_with_zeros, window_size)
          return x_data, y_data
        
By leveraging the divided dataset into training and testing representing empirical input-output responses, the CNN-based black-box model can effectively learn and replicate the nonlinear behavior of IBRs, making it a valuable tool for power system analysis and control. 

### CNN Architecture

#### 1\. Data Preprocessing

  * Convert time-series data into a 2D structure using sliding windows.
  * Organize measurement signals into multiple input channels.
#### 2\. CNN Layer Design

  * **1D Convolutional Layers** : Extract features from input signals using multiple filters.
  * **Batch Normalization** : Normalize outputs of convolutional layers to improve training.
  * **Activation Functions (ReLU/ELU)** : Introduce non-linearity to enhance learning.
  * **Pooling Layers** : Reduce dimensionality while retaining important features.
  * **Fully Connected (Dense) Layers** : Flatten the output from pooling layers and map features to the output response.
  * **Output Layer** : Use an appropriate layer for regression or classification tasks.
#### 3\. Training

  * Train the model with a suitable loss function, optimizer, and evaluation metric.
  * Fine-tune hyperparameters (e.g., learning rate, batch size) for optimal performance.
#### 4\. Model Evaluation

  * Evaluate the model on test data and adjust as needed based on the metrics such as Mean Absolute Error (MAE) or Mean Squared Error (MSE).
#### 5\. Model Deployment

  * Deploy the model for real-time prediction or further analysis after satisfactory performance.
  
Pseudocode for the CNN model training and testing: 
    
    
    # 1. Preprocess the Data
    function reshape_data(data, window_size):
       x_data = []  # Input data (sequences of past observations)
       y_data = []  # Output data (future observations or averages)
       # Add zeros at the beginning for the initial conditions
       data_with_zeros = prepend_zeros(data, window_size)
       # Reshape input data
       x_data = create_input_windows(data_with_zeros, window_size)
       # Reshape output data and take the average for each window
       y_data = create_output_windows(data_with_zeros, window_size)
       return x_data, y_data
    # 2. Train the Model
    function train_model(train_data, train_labels):
       model = create_model()  # A basic CNN model is created
       model.fit(train_data, train_labels)  # Train the model with data
       return model
    # 3. Save the Model
    function save_model(model, filename="model.keras"):
       model.save(filename)  # Save the model to file
    # 4. Load the Model
    function load_model(filename="model.keras"):
       model = load(filename)  # Load the model from file
       return model
    # 5. Evaluate the Model
    function test_model(model, test_data, test_labels):
       evaluation = model.evaluate(test_data, test_labels)  # Test the model
       print("Test Loss:", evaluation.loss)
       print("Test Accuracy:", evaluation.accuracy)
    # Main Execution
    # 1. Load raw data
     raw_data = load_data("data.csv")
    # 2. Reshape data into windows
     window_size = 50
     x_data, y_data = reshape_data(raw_data, window_size)
    # 3. Train the model with the training data
     model = train_model(x_data, y_data)
    # 4. Save the trained model
     save_model(model)
    # 5. Load the saved model
     loaded_model = load_model("model.keras")
    # 6. Test the loaded model with test data
     test_data, test_labels = load_test_data("test_data.csv")
     test_model(loaded_model, test_data, test_labels)
    

  


## Open-Source Integration: Prediction with CNN

A Keras model saved in the .keras format includes everything needed to reload and continue using the model. It stores the model architecture, trained weights, optimizer state, training configuration, and any custom objects. This format ensures seamless reloading without needing extra code to reconstruct the model. The saved model is then translated into the C++ programming language in the form of a JSON file. To streamline the model conversion process, a third-party C++ library was utilized, Frugally-deep. This library facilitated the conversion of the black-box model into the Frugally-deep file format (.json), allowing for its subsequent loading and execution within a C++ environment. The details of the header library that uses keras models in C++ can be found here [[[2]](https://github.com/Dobiasd/frugally-deep)]. Code below provides the general instructions how the trained model can be translated to C++ code for to integrate into a opensource like GridLAB-D™. 

  

    
    
    model.save('keras_model.keras')
    python3 keras_export/convert_model.py keras_model.keras fdeep_model.json
    // main.cpp
    #include <fdeep/fdeep.hpp>
    int main()
    {
       const auto model = fdeep::load_model("fdeep_model.json");
       const auto result = model.predict(
           {fdeep::tensor(fdeep::tensor_shape(static_cast<std::size_t>(4)),
           std::vector<float>{1, 2, 3, 4})});
       std::cout << fdeep::show_tensors(result) << std::endl;
    }
    

  


## Requirement and Installations
    
    
    === 1. Install and Import Required Libraries ===
    First, ensure you have the necessary packages installed:
    * `pip install tensorflow numpy pandas matplotlib scikit-learn`
    More Info at: <https://www.tensorflow.org/install>
    
    
    
    Now, import the required libraries:
    * `import numpy as np`
    * `import pandas as pd`
    * `import tensorflow as tf`
    * `from tensorflow import keras`
    * `from tensorflow.keras.models import Sequential`
    * `from tensorflow.keras.layers import Conv1D, Dense, Flatten, Dropout, BatchNormalization, MaxPooling1D`
    * `from sklearn.preprocessing import MinMaxScaler`
    * `import matplotlib.pyplot as plt`
    
    
    
    === Third Party Library Requirement and Installations ===
    * A C++14-compatible compiler: Compilers from these versions on are fine: GCC 4.9, Clang 3.7 (libc++ 3.7) and Visual C++ 2015
    * Python 3.9 or higher
    * TensorFlow 2.18.0 or higher
    * Keras 3.8.0 or higher
    
    
    
    More Info at: <https://github.com/Dobiasd/frugally-deep>
    

  


## Pros, Cons, and Limitations

### Pros

  * Adaptability to Complex Systems: Machine Learning models excel at capturing nonlinear and complex dynamics of IBR without requiring detailed physical equations.
  * Real-Time Applicability: Once trained, these models can process large datasets quickly, enabling real-time monitoring, prediction, or control of IBR dynamics in modern smart grids.
  * Pattern Recognition: They can identify hidden patterns or anomalies in IBR behavior (e.g., fault detection) from historical operational data, improving system reliability.
  * Scalability: Data-driven models can generalize across different IBR types or grid conditions, provided sufficient training data is available.
### Cons

  * Data Dependency: These models require large, high-quality datasets. Incomplete, noisy, or biased data (e.g., from limited operating conditions) can degrade performance.
  * Lack of Interpretability: Unlike physics-based models, data-driven approaches (especially black-box models like neural networks) often provide little insight into why certain dynamics occur, which can hinder trust or debugging.
  * Overfitting Risk: Without careful tuning, these models may fit training data too closely and fail to generalize to new scenarios, such as rare grid events (e.g., blackouts).
  * Computational Cost: Training sophisticated models (e.g., deep learning) can be resource-intensive, requiring significant time and hardware, even if inference is fast.
### Limitations

  * Dynamic System Challenges: IBR dynamics are time-varying (e.g., due to changing weather or grid demand), and purely data-driven models may lag in capturing rapid shifts unless continuously updated.
  * Validation Difficulty: Without a physics-based benchmark, validating model accuracy across all operating conditions is challenging, especially for rare or untested scenarios.
  * Regulatory and Safety Concerns: In power systems, where reliability is critical, the "black-box" nature of some data-driven models may not meet strict certification or explainability requirements.
  


## References

  1. [S. Subedi, Y. Gui, and Y. Xue, "Applications of Data-Driven Dynamic Modeling of Power Converters in Power Systems: An Overview," in IEEE Transactions on Industry Applications, vol. 61, no. 2, pp. 2434-2456, Jan. 2025.](https://ieeexplore.ieee.org/abstract/document/10839589)
  2. [S. Subedi, L. Qiao, Y. Gui, Y. Xue, F. Tuffner, W. Du, "Deep Learning-Based Dynamic Modeling of Three-Phase Voltage Source Inverters," 2023 IEEE Energy Conversion Congress and Exposition (ECCE), Phoenix, AZ, USA, 2024, pp. 4450-4456.](https://ieeexplore.ieee.org/abstract/document/10861015)
  


## Related Concepts:

  * Inverter_dyn Main Page
  * Inverter_dyn Requirements
  * Inverter_dyn Specifications
