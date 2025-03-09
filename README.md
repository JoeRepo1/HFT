Small trading app to demo a few common HFT design components and relevant C++20 features.
-----------------------------------------------------------------------------------------

The project demonstrates a hybrid trading system that integrates multiple algorithmic strategies, dynamically selecting and blending their outputs based on real-time market conditions.

## Disclaimer
This is a demonstration project intended for educational purposes and is not suitable for production deployment. Market data and exchange connectivity are represented by placeholder functions.

## Trading App
- **Strategies**:
  - Pair Trading: Exploits statistical arbitrage opportunities between correlated assets.
  - Microstructure Alpha: Leverages order flow and market microstructure signals.
  - Order Book Imbalance: Detects directional pressure from order book dynamics.
  - Momentum Ignition: Identifies and capitalizes on short-term momentum bursts.
  - Volatility Breakout: Trades based on significant volatility expansions.
  - Custom "Hot Pattern" Algorithm: Activates under predefined market conditions (e.g., tight spreads, elevated volatility) to optimize profitability.

- **Signal Blending Mechanism**: Employs an Exponentially Weighted Moving Average (EWMA) to aggregate strategy signals, with adaptive weighting to emphasize the custom algorithm during anomalous market states.

- **Performance Optimizations**:
  - Single Instruction, Multiple Data (SIMD) instructions via AVX2 for vectorized signal processing and EWMA calculations.
  - Aligned memory buffers to enhance cache efficiency and data access speeds.

- **Supporting Components**:
  - Asynchronous logging system for non-blocking diagnostic output.
  - Connection factory abstraction for managing simulated exchange interfaces, designed with low-latency considerations.

## System Requirements
- **Compiler**: Requires a C++20-compliant compiler, such as:
  - GCC 10 or later
  - Clang 10 or later
  - Microsoft Visual Studio 2019 (version 16.8+) or later
- **Hardware**: Processor supporting AVX2 instructions for SIMD functionality.

## Build Instructions
To compile the project, clone the repository and use a compatible C++20 compiler. The following commands assume a Unix-like environment:

```bash
git clone https://github.com/yourusername/yourrepo.git
cd yourrepo
g++ -std=c++20 -O3 -march=native main.cpp -o trading_daemon
```

If the project includes additional source files (e.g., `AsyncLogger.h`, `ConnectionFactory.h`), ensure they are included in the compilation command, such as:

```bash
g++ -std=c++20 -O3 -march=native *.cpp -o trading_daemon
```
The `-O3` flag enables aggressive optimization, and `-march=native` ensures the compiler leverages the host CPU's instruction set, including AVX2 where available.

## Execution
Run the compiled binary as follows:

```bash
./trading_daemon
```

The program executes a single cycle of the hybrid trading algorithm, processing simulated market data, generating and blending signals, and conditionally executing a trade. In a production context, this would operate continuously with live market feeds.

## Architecture Overview
The daemon adheres to a modular structure, with the following workflow:

1. **Market Feature Extraction**: Processes input data to compute quantitative features, including volatility, trend direction, order book imbalance, and liquidity metrics.
2. **Strategy Signal Generation**: Each trading algorithm independently evaluates the features to produce a signal.
3. **Signal Aggregation**: Applies an EWMA-based blending technique to combine signals, dynamically adjusting weights based on market conditions.
4. **Trade Decision and Execution**: Assesses the aggregated signal against a predefined threshold; if exceeded, simulates trade execution with risk-adjusted parameters (e.g., order size, price).

The custom "Hot Pattern" algorithm activates under specific conditions—such as high volatility coupled with tight spreads and latent liquidity—enhancing potential returns.

## License
This project is distributed under the MIT License. Refer to the LICENSE file for full terms and conditions.
