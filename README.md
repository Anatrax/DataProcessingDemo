Data Processing Proof of Concept
=================================
This project demonstrates a python "Data Processing" block reading in simulated flight data and playing back the data packets in real time as a message to a "GUI" block written in C which then parses the message into usable variables.

These "Data Processing" and "GUI" programs are just stand-ins for the final programs and only demonstrate the messaging interface between these components as well as the play-back capabilities for replaying simulated data as if it were live flight telemetry.

Prerequisites
--------------
- Make
- Python 3

Building and Running the Example
---------------------------------
1. Build code
    ```bash
    $ make
    ```
2. Start server
    ```bash
    # In another window...
    $ python3 src/server.py simulations/booster_sim.csv 8888
    ```
3. Start client
    ```bash
    $ ./bin/data_processing_proof_of_concept 8888
    ```

Roadmap
--------
- Switch C code to C++
- Clean up message parsing code
- Send all of the necessary variables (currently only supports a few time data points)
- Store variables in hash map indexed by the given label
- Interface with the actual GUI code
- Read in and play back recorded telemetry and flight data (.telem and .eeprom)
- Read in live flight telemetry
- Implement some data processing to smooth out noisy data and provide a more accurate and precise estimate of the rocket's actual state

