#ifndef ANGELONE_WEBSOCKET_H
#define ANGELONE_WEBSOCKET_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Forward declarations for websocketpp
namespace websocketpp {
    namespace lib {
        namespace error_code = lib::error_code;
    }
    class connection_hdl;
}

class AngelOneWebSocket {
public:
    // Structure to hold market data
    struct MarketData {
        std::string symbol;
        std::string exchange;
        double last_price;
        double open_price;
        double high_price;
        double low_price;
        double close_price;
        int64_t volume;
        int64_t oi;
        std::string timestamp;
        std::string mode;
    };

    // Callback types
    using OnConnectCallback = std::function<void()>;
    using OnDisconnectCallback = std::function<void(int, const std::string&)>;
    using OnMessageCallback = std::function<void(const std::string&)>;
    using OnErrorCallback = std::function<void(const std::string&)>;

    AngelOneWebSocket();
    ~AngelOneWebSocket();

    // Configuration
    void setCredentials(const std::string& api_key, 
                       const std::string& access_token,
                       const std::string& client_id);
    
    void setLogFilePaths(const std::string& txt_file, const std::string& csv_file);
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;

    // Subscription management
    bool subscribe(const std::vector<std::pair<std::string, std::string>>& tokens);
    bool unsubscribe(const std::vector<std::pair<std::string, std::string>>& tokens);

    // Callbacks
    void setOnConnectCallback(OnConnectCallback callback);
    void setOnDisconnectCallback(OnDisconnectCallback callback);
    void setOnMessageCallback(OnMessageCallback callback);
    void setOnErrorCallback(OnErrorCallback callback);

private:
    // Internal methods
    void initializeWebsocket();
    void onOpen(websocketpp::connection_hdl hdl);
    void onFail(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, 
                   websocketpp::config::asio_client::message_type::ptr msg);
    
    void parseAndLogData(const std::string& message);
    void writeToFile(const MarketData& data);
    void initializeCsvFile();
    std::string getCurrentTimestamp();
    std::string generateAuthHeader();

    // Credentials
    std::string m_api_key;
    std::string m_access_token;
    std::string m_client_id;

    // File paths
    std::string m_txt_log_path;
    std::string m_csv_log_path;

    // Websocket endpoint and connection
    std::unique_ptr<void*> m_endpoint; // Using void* to avoid header dependency in .h
    websocketpp::connection_hdl m_connection_hdl;
    bool m_connected;
    bool m_csv_initialized;

    // Callbacks
    OnConnectCallback m_on_connect;
    OnDisconnectCallback m_on_disconnect;
    OnMessageCallback m_on_message;
    OnErrorCallback m_on_error;

    // Thread safety
    mutable std::mutex m_mutex;
    std::ofstream m_txt_file;
    std::ofstream m_csv_file;

    // Constants
    static constexpr const char* WEBSOCKET_URL = "wss://smartapi.angelbroking.com/NestHtml5Mobile/socket/stream";
};

#endif // ANGELONE_WEBSOCKET_H
