# AngelOne WebSocket Client - C++

A complete C++ implementation for connecting to AngelOne SmartAPI WebSocket for real-time market data streaming.

## Features

- ✅ Complete WebSocket client implementation
- ✅ Login/authentication support
- ✅ Subscribe/Unsubscribe to market symbols
- ✅ Real-time data logging to TXT and CSV files
- ✅ Thread-safe operations
- ✅ Error handling and reconnection support
- ✅ Callback-based event handling

## Prerequisites

### Install Required Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    libboost-all-dev \
    libssl-dev \
    libwebsocketpp-dev \
    nlohmann-json3-dev \
    cmake \
    g++ \
    libasio-dev
```

**macOS (using Homebrew):**
```bash
brew install boost openssl websocketpp nlohmann-json cmake
```

**Windows (using vcpkg):**
```bash
vcpkg install boost websocketpp nlohmann-json openssl
```

## Project Structure

```
/workspace
├── angelone_websocket.h      # Header file with class definition
├── angelone_websocket.cpp    # Implementation file
├── main.cpp                  # Example usage
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── market_data.txt          # Output: Text log file (generated)
└── market_data.csv          # Output: CSV data file (generated)
```

## Building the Project

### Using CMake (Recommended)

```bash
cd /workspace
mkdir build
cd build
cmake ..
make
```

### Manual Compilation

```bash
g++ -std=c++17 -pthread angelone_websocket.cpp main.cpp \
    -lboost_system -lssl -lcrypto -lz \
    -o angelone_client
```

## Configuration

### Get Your AngelOne Credentials

1. **API Key**: Login to [AngelOne SmartAPI](https://smartapi.angelbroking.com/) and get your API key
2. **Client ID**: Your AngelOne trading client ID
3. **Access Token**: Generate using the login API (see below)

### Generate Access Token

You need to first authenticate and get an access token. Here's a simple example:

```cpp
// You can use any HTTP library like libcurl or cpp-httplib
// Example pseudo-code:
string generateAccessToken(const string& apiKey, const string& clientId, const string& password, const string&totp) {
    // POST to: https://apiconnect.angelbroking.com/rest/auth/angelbroking/user/v1/loginByPassword
    // Returns access_token in response
}
```

## Usage

### Basic Example

```cpp
#include "angelone_websocket.h"

int main() {
    AngelOneWebSocket ws;
    
    // Set credentials
    ws.setCredentials("your_api_key", "your_access_token", "your_client_id");
    
    // Set output files
    ws.setLogFilePaths("market_data.txt", "market_data.csv");
    
    // Set callbacks (optional)
    ws.setOnConnectCallback([]() {
        std::cout << "Connected!" << std::endl;
    });
    
    ws.setOnMessageCallback([](const std::string& msg) {
        std::cout << "Received: " << msg << std::endl;
    });
    
    // Connect
    if (!ws.connect()) {
        std::cerr << "Connection failed!" << std::endl;
        return 1;
    }
    
    // Subscribe to symbols
    std::vector<std::pair<std::string, std::string>> symbols = {
        {"NSE", "11536"},      // NIFTY 50
        {"NSE", "99926000"},   // BANKNIFTY
        {"BSE", "4719"}        // SENSEX
    };
    
    ws.subscribe(symbols);
    
    // Keep running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Cleanup
    ws.unsubscribe(symbols);
    ws.disconnect();
    
    return 0;
}
```

### Running the Example

```bash
# With command line arguments
./build/angelone_client your_api_key your_access_token your_client_id

# Or edit main.cpp and recompile with your credentials
```

## Symbol Tokens

You need to use the correct symbol tokens for the instruments you want to subscribe to. 

### Common Index Tokens:
- NIFTY 50: `11536` (NSE)
- BANKNIFTY: `99926000` (NSE)
- SENSEX: `4719` (BSE)
- FINNIFTY: `28813` (NSE)

### Getting Symbol Tokens

Use the AngelOne API to fetch the complete list:
```bash
curl -X GET "https://apiconnect.angelbroking.com/rest/secure/angelbroking/market/v1/quote" \
  -H "Authorization: Bearer YOUR_ACCESS_TOKEN"
```

Or refer to the [AngelOne SmartAPI documentation](https://smartapi.angelbroking.com/docs).

## Output Files

### market_data.txt (Text Format)
```
[2024-01-15 10:30:45.123] Connected to AngelOne WebSocket
[2024-01-15 10:30:46.456] RAW: {"symbol":"11536","exchange":"NSE","ltp":21543.50,...}
[2024-01-15 10:30:46.457] Symbol: 11536 | Exchange: NSE | LTP: 21543.50 | ...
```

### market_data.csv (CSV Format)
```csv
timestamp,symbol,exchange,last_price,open_price,high_price,low_price,close_price,volume,oi,mode
2024-01-15 10:30:46.457,11536,NSE,21543.50,21500.00,21600.00,21480.00,21520.00,1234567,0,live
```

## API Reference

### AngelOneWebSocket Class

#### Methods

| Method | Description |
|--------|-------------|
| `setCredentials(api_key, access_token, client_id)` | Set authentication credentials |
| `setLogFilePaths(txt_file, csv_file)` | Configure output file paths |
| `connect()` | Establish WebSocket connection |
| `disconnect()` | Close connection |
| `isConnected()` | Check connection status |
| `subscribe(tokens)` | Subscribe to market symbols |
| `unsubscribe(tokens)` | Unsubscribe from symbols |

#### Callbacks

| Callback | Parameters | Description |
|----------|-----------|-------------|
| `setOnConnectCallback` | `void()` | Called on successful connection |
| `setOnDisconnectCallback` | `int code, string reason` | Called on disconnection |
| `setOnMessageCallback` | `string message` | Called on receiving data |
| `setOnErrorCallback` | `string error` | Called on errors |

## Troubleshooting

### Connection Issues

1. **Check credentials**: Ensure API key, access token, and client ID are correct
2. **Network**: Verify internet connection and firewall settings
3. **Token expiry**: Access tokens expire daily, generate a new one

### Compilation Errors

- **Missing headers**: Install all dependencies listed in Prerequisites
- **C++ standard**: Ensure compiler supports C++17 or later
- **Linker errors**: Make sure all libraries are linked correctly

### Runtime Errors

- **Authentication failed**: Regenerate access token
- **Invalid symbol tokens**: Verify symbol tokens from AngelOne API
- **Rate limiting**: Don't subscribe to too many symbols at once

## Security Notes

⚠️ **Important Security Practices:**

1. Never commit credentials to version control
2. Use environment variables for sensitive data
3. Regenerate access tokens regularly
4. Implement proper error handling in production

## License

This code is provided as-is for educational purposes. Use at your own risk.

## Support

For AngelOne API issues, refer to:
- [SmartAPI Documentation](https://smartapi.angelbroking.com/docs)
- [AngelOne Developer Portal](https://smartapi.angelbroking.com/)

## Contributing

Feel free to submit issues and enhancement requests!
