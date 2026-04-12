#include "angelone_websocket.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "========================================" << std::endl;
    std::cout << "AngelOne WebSocket Client - C++" << std::endl;
    std::cout << "========================================" << std::endl;

    // Check for command line arguments or use defaults
    std::string api_key = "your_api_key";
    std::string access_token = "your_access_token";
    std::string client_id = "your_client_id";

    if (argc > 3) {
        api_key = argv[1];
        access_token = argv[2];
        client_id = argv[3];
    } else {
        std::cout << "\nUsage: " << argv[0] << " <api_key> <access_token> <client_id>" << std::endl;
        std::cout << "Using default credentials (you should replace these!)" << std::endl;
        std::cout << "\nIMPORTANT: Replace the credentials in the code with your actual AngelOne credentials:" << std::endl;
        std::cout << "  - API Key: Get from AngelOne SmartAPI dashboard" << std::endl;
        std::cout << "  - Access Token: Generate using login API" << std::endl;
        std::cout << "  - Client ID: Your AngelOne client ID" << std::endl;
        std::cout << "\nYou can also pass them as command line arguments." << std::endl;
    }

    // Create WebSocket instance
    AngelOneWebSocket ws;

    // Set credentials
    ws.setCredentials(api_key, access_token, client_id);

    // Set log file paths
    ws.setLogFilePaths("market_data.txt", "market_data.csv");

    // Set callbacks
    ws.setOnConnectCallback([]() {
        std::cout << "[Callback] Connected successfully!" << std::endl;
    });

    ws.setOnDisconnectCallback([](int code, const std::string& reason) {
        std::cout << "[Callback] Disconnected. Code: " << code << ", Reason: " << reason << std::endl;
    });

    ws.setOnMessageCallback([](const std::string& message) {
        std::cout << "[Callback] Received message (length: " << message.length() << " bytes)" << std::endl;
    });

    ws.setOnErrorCallback([](const std::string& error) {
        std::cerr << "[Callback] Error: " << error << std::endl;
    });

    // Connect to WebSocket
    std::cout << "\nAttempting to connect to AngelOne WebSocket..." << std::endl;
    if (!ws.connect()) {
        std::cerr << "Failed to connect. Please check your credentials and internet connection." << std::endl;
        return 1;
    }

    std::cout << "Connected successfully!" << std::endl;
    std::cout << "Data will be logged to:" << std::endl;
    std::cout << "  - TXT: market_data.txt" << std::endl;
    std::cout << "  - CSV: market_data.csv" << std::endl;

    // Subscribe to symbols
    // Format: {exchange, symbol_token}
    // Common exchanges: NSE, BSE, NFO, MCX, CDS
    // You need to get the correct symbol tokens from AngelOne API
    std::vector<std::pair<std::string, std::string>> symbols = {
        {"NSE", "11536"},   // NIFTY 50
        {"NSE", "99926000"}, // BANKNIFTY
        {"BSE", "4719"},    // SENSEX
        // Add more symbols as needed
        // Example: {"NSE", "RELIANCE-EQ"}, {"NSE", "TCS-EQ"}, etc.
    };

    std::cout << "\nSubscribing to " << symbols.size() << " symbols..." << std::endl;
    if (ws.subscribe(symbols)) {
        std::cout << "Subscription successful!" << std::endl;
    } else {
        std::cerr << "Subscription failed!" << std::endl;
    }

    // Keep running and wait for data
    std::cout << "\nListening for market data... Press Ctrl+C to stop." << std::endl;
    
    int seconds = 0;
    while (g_running && ws.isConnected()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        seconds++;
        
        if (seconds % 10 == 0) {
            std::cout << "." << std::flush;
        }
        
        // Optional: You can add reconnection logic here
        // Or perform other tasks while receiving data
    }

    // Unsubscribe before disconnecting
    std::cout << "\n\nUnsubscribing from symbols..." << std::endl;
    ws.unsubscribe(symbols);

    // Disconnect
    std::cout << "Disconnecting..." << std::endl;
    ws.disconnect();

    std::cout << "\n========================================" << std::endl;
    std::cout << "Session ended." << std::endl;
    std::cout << "Check market_data.txt and market_data.csv for logged data." << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
