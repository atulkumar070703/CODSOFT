#include "angelone_websocket.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <future>

// Type definitions for websocketpp
typedef websocketpp::client<websocketpp::config::asio_tls_client> WsClient;
typedef websocketpp::message_buffer::message<websocketpp::message_buffer::alloc::con_msg_alloc> WsMessage;

// Include implementation details in an anonymous namespace to keep header clean
namespace {
    // Helper to convert connection_hdl to void* for storage
    struct ConnectionHdlWrapper {
        websocketpp::connection_hdl hdl;
    };
}

class AngelOneWebSocket::Impl {
public:
    WsClient endpoint;
    websocketpp::connection_hdl connection_hdl;
    bool connected = false;
    std::promise<bool> connect_promise;
    
    Impl() {
        endpoint.init_asio();
        endpoint.set_access_channels(websocketpp::log::alevel::none);
        endpoint.set_error_channels(websocketpp::log::elevel::none);
        
        // Configure TLS
        endpoint.set_tls_init_handler([](websocketpp::connection_hdl) {
            auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
            try {
                ctx->set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::single_dh_use);
            } catch (const std::exception& e) {
                std::cerr << "TLS init error: " << e.what() << std::endl;
            }
            return ctx;
        });
        
        // Set handlers
        endpoint.set_open_handler([this](websocketpp::connection_hdl hdl) {
            connection_hdl = hdl;
            connected = true;
            connect_promise.set_value(true);
        });
        
        endpoint.set_fail_handler([this](websocketpp::connection_hdl hdl) {
            connected = false;
            if (!connect_promise.get_future().is_ready()) {
                connect_promise.set_value(false);
            }
        });
        
        endpoint.set_close_handler([this](websocketpp::connection_hdl hdl) {
            connected = false;
        });
    }
};

AngelOneWebSocket::AngelOneWebSocket() 
    : m_connected(false), m_csv_initialized(false) {
    m_txt_log_path = "market_data.txt";
    m_csv_log_path = "market_data.csv";
    initializeWebsocket();
}

AngelOneWebSocket::~AngelOneWebSocket() {
    disconnect();
    if (m_txt_file.is_open()) {
        m_txt_file.close();
    }
    if (m_csv_file.is_open()) {
        m_csv_file.close();
    }
}

void AngelOneWebSocket::initializeWebsocket() {
    // Implementation will use websocketpp internally
    // This is a placeholder - actual implementation requires websocketpp headers
}

void AngelOneWebSocket::setCredentials(const std::string& api_key, 
                                       const std::string& access_token,
                                       const std::string& client_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_api_key = api_key;
    m_access_token = access_token;
    m_client_id = client_id;
}

void AngelOneWebSocket::setLogFilePaths(const std::string& txt_file, const std::string& csv_file) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_txt_log_path = txt_file;
    m_csv_log_path = csv_file;
    
    // Open files
    if (m_txt_file.is_open()) {
        m_txt_file.close();
    }
    m_txt_file.open(m_txt_log_path, std::ios::app);
    
    if (m_csv_file.is_open()) {
        m_csv_file.close();
    }
    m_csv_file.open(m_csv_log_path, std::ios::app);
    
    if (!m_csv_initialized) {
        initializeCsvFile();
    }
}

bool AngelOneWebSocket::connect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_connected) {
        return true;
    }
    
    if (m_api_key.empty() || m_access_token.empty() || m_client_id.empty()) {
        if (m_on_error) {
            m_on_error("Credentials not set. Call setCredentials first.");
        }
        return false;
    }
    
    try {
        // Create endpoint
        auto* ep = new WsClient();
        m_endpoint.reset(ep);
        
        WsClient* client = static_cast<WsClient*>(m_endpoint.get());
        
        // Initialize ASIO
        client->init_asio();
        
        // Disable logging for performance
        client->set_access_channels(websocketpp::log::alevel::none);
        client->set_error_channels(websocketpp::log::elevel::warn);
        
        // Set TLS handler
        client->set_tls_init_handler([](websocketpp::connection_hdl) {
            auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
            try {
                ctx->set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::single_dh_use);
            } catch (const std::exception& e) {
                std::cerr << "TLS init error: " << e.what() << std::endl;
            }
            return ctx;
        });
        
        // Bind handlers
        client->set_open_handler([this](websocketpp::connection_hdl hdl) {
            onOpen(hdl);
        });
        
        client->set_fail_handler([this](websocketpp::connection_hdl hdl) {
            onFail(hdl);
        });
        
        client->set_close_handler([this](websocketpp::connection_hdl hdl) {
            onClose(hdl);
        });
        
        client->set_message_handler([this](websocketpp::connection_hdl hdl, 
                                          WsClient::message_ptr msg) {
            onMessage(hdl, msg);
        });
        
        // Connect
        websocketpp::lib::error_code ec;
        auto con = client->get_connection(WEBSOCKET_URL, ec);
        
        if (ec) {
            if (m_on_error) {
                m_on_error("Get connection error: " + ec.message());
            }
            return false;
        }
        
        // Add authorization header
        con->add_header("Authorization", generateAuthHeader());
        con->add_header("x-user-agent", "Desktop");
        con->add_header("x-client-source", "web");
        
        client->connect(con);
        
        // Start ASIO loop in separate thread
        std::thread asio_thread([client]() {
            client->run();
        });
        asio_thread.detach();
        
        // Wait for connection
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        return m_connected;
        
    } catch (const std::exception& e) {
        if (m_on_error) {
            m_on_error(std::string("Connection error: ") + e.what());
        }
        return false;
    }
}

void AngelOneWebSocket::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_connected || !m_endpoint) {
        return;
    }
    
    try {
        WsClient* client = static_cast<WsClient*>(m_endpoint.get());
        websocketpp::lib::error_code ec;
        client->close(m_connection_hdl, websocketpp::close::status::normal, "User disconnect", ec);
        
        if (ec) {
            std::cerr << "Close error: " << ec.message() << std::endl;
        }
        
        m_connected = false;
        
    } catch (const std::exception& e) {
        std::cerr << "Disconnect error: " << e.what() << std::endl;
    }
}

bool AngelOneWebSocket::isConnected() const {
    return m_connected;
}

bool AngelOneWebSocket::subscribe(const std::vector<std::pair<std::string, std::string>>& tokens) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_connected) {
        if (m_on_error) {
            m_on_error("Cannot subscribe: Not connected");
        }
        return false;
    }
    
    try {
        WsClient* client = static_cast<WsClient*>(m_endpoint.get());
        
        // Build subscription message
        nlohmann::json j;
        j["msgtype"] = "nw";
        j["source"] = "API";
        j["channel"] = "";
        j["token"] = m_access_token;
        
        nlohmann::json symbols = nlohmann::json::array();
        for (const auto& token : tokens) {
            nlohmann::json sym;
            sym["exch"] = token.first;  // Exchange (NFO, BSE, NSE, etc.)
            sym["symbol"] = token.second; // Symbol token
            symbols.push_back(sym);
        }
        j["subscription"] = symbols;
        
        std::string msg = j.dump();
        client->send(m_connection_hdl, msg, websocketpp::frame::opcode::text);
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_on_error) {
            m_on_error(std::string("Subscribe error: ") + e.what());
        }
        return false;
    }
}

bool AngelOneWebSocket::unsubscribe(const std::vector<std::pair<std::string, std::string>>& tokens) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_connected) {
        if (m_on_error) {
            m_on_error("Cannot unsubscribe: Not connected");
        }
        return false;
    }
    
    try {
        WsClient* client = static_cast<WsClient*>(m_endpoint.get());
        
        // Build unsubscription message
        nlohmann::json j;
        j["msgtype"] = "nd";
        j["source"] = "API";
        j["channel"] = "";
        j["token"] = m_access_token;
        
        nlohmann::json symbols = nlohmann::json::array();
        for (const auto& token : tokens) {
            nlohmann::json sym;
            sym["exch"] = token.first;
            sym["symbol"] = token.second;
            symbols.push_back(sym);
        }
        j["unsubscription"] = symbols;
        
        std::string msg = j.dump();
        client->send(m_connection_hdl, msg, websocketpp::frame::opcode::text);
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_on_error) {
            m_on_error(std::string("Unsubscribe error: ") + e.what());
        }
        return false;
    }
}

void AngelOneWebSocket::setOnConnectCallback(OnConnectCallback callback) {
    m_on_connect = callback;
}

void AngelOneWebSocket::setOnDisconnectCallback(OnDisconnectCallback callback) {
    m_on_disconnect = callback;
}

void AngelOneWebSocket::setOnMessageCallback(OnMessageCallback callback) {
    m_on_message = callback;
}

void AngelOneWebSocket::setOnErrorCallback(OnErrorCallback callback) {
    m_on_error = callback;
}

void AngelOneWebSocket::onOpen(websocketpp::connection_hdl hdl) {
    m_connection_hdl = hdl;
    m_connected = true;
    
    std::cout << "[WebSocket] Connected to AngelOne" << std::endl;
    
    if (m_txt_file.is_open()) {
        m_txt_file << "[" << getCurrentTimestamp() << "] Connected to AngelOne WebSocket\n";
        m_txt_file.flush();
    }
    
    if (m_on_connect) {
        m_on_connect();
    }
}

void AngelOneWebSocket::onFail(websocketpp::connection_hdl hdl) {
    m_connected = false;
    
    std::string error_msg = "Connection failed";
    std::cerr << "[WebSocket] " << error_msg << std::endl;
    
    if (m_txt_file.is_open()) {
        m_txt_file << "[" << getCurrentTimestamp() << "] Connection failed\n";
        m_txt_file.flush();
    }
    
    if (m_on_error) {
        m_on_error(error_msg);
    }
}

void AngelOneWebSocket::onClose(websocketpp::connection_hdl hdl) {
    m_connected = false;
    
    std::cout << "[WebSocket] Disconnected from AngelOne" << std::endl;
    
    if (m_txt_file.is_open()) {
        m_txt_file << "[" << getCurrentTimestamp() << "] Disconnected from AngelOne WebSocket\n";
        m_txt_file.flush();
    }
    
    if (m_on_disconnect) {
        m_on_disconnect(0, "Closed");
    }
}

void AngelOneWebSocket::onMessage(websocketpp::connection_hdl hdl, 
                                  WsClient::message_ptr msg) {
    std::string payload = msg->get_payload();
    
    // Log raw message
    if (m_txt_file.is_open()) {
        m_txt_file << "[" << getCurrentTimestamp() << "] RAW: " << payload << "\n";
        m_txt_file.flush();
    }
    
    // Parse and process
    parseAndLogData(payload);
    
    // Callback
    if (m_on_message) {
        m_on_message(payload);
    }
}

void AngelOneWebSocket::parseAndLogData(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        
        // Check if it's a feed message
        if (j.contains("feed_type") || j.contains("data")) {
            MarketData data;
            
            // Parse based on AngelOne format
            if (j.contains("data") && j["data"].is_array()) {
                for (const auto& item : j["data"]) {
                    if (item.contains("symbol")) {
                        data.symbol = item.value("symbol", "");
                    }
                    if (item.contains("exchange")) {
                        data.exchange = item.value("exchange", "");
                    }
                    if (item.contains("ltp")) {
                        data.last_price = item.value("ltp", 0.0);
                    }
                    if (item.contains("open")) {
                        data.open_price = item.value("open", 0.0);
                    }
                    if (item.contains("high")) {
                        data.high_price = item.value("high", 0.0);
                    }
                    if (item.contains("low")) {
                        data.low_price = item.value("low", 0.0);
                    }
                    if (item.contains("close")) {
                        data.close_price = item.value("close", 0.0);
                    }
                    if (item.contains("volume")) {
                        data.volume = item.value("volume", int64_t(0));
                    }
                    if (item.contains("oi")) {
                        data.oi = item.value("oi", int64_t(0));
                    }
                    
                    data.timestamp = getCurrentTimestamp();
                    data.mode = "live";
                    
                    writeToFile(data);
                }
            } else if (j.contains("symbol")) {
                // Single symbol format
                if (j.contains("symbol")) data.symbol = j.value("symbol", "");
                if (j.contains("exchange")) data.exchange = j.value("exchange", "");
                if (j.contains("ltp")) data.last_price = j.value("ltp", 0.0);
                if (j.contains("open")) data.open_price = j.value("open", 0.0);
                if (j.contains("high")) data.high_price = j.value("high", 0.0);
                if (j.contains("low")) data.low_price = j.value("low", 0.0);
                if (j.contains("close")) data.close_price = j.value("close", 0.0);
                if (j.contains("volume")) data.volume = j.value("volume", int64_t(0));
                if (j.contains("oi")) data.oi = j.value("oi", int64_t(0));
                
                data.timestamp = getCurrentTimestamp();
                data.mode = "live";
                
                writeToFile(data);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[Parse Error] " << e.what() << std::endl;
        if (m_txt_file.is_open()) {
            m_txt_file << "[" << getCurrentTimestamp() << "] Parse Error: " << e.what() << "\n";
            m_txt_file.flush();
        }
    }
}

void AngelOneWebSocket::writeToFile(const MarketData& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Write to TXT file
    if (m_txt_file.is_open()) {
        m_txt_file << "[" << data.timestamp << "] "
                   << "Symbol: " << data.symbol << " | "
                   << "Exchange: " << data.exchange << " | "
                   << "LTP: " << data.last_price << " | "
                   << "Open: " << data.open_price << " | "
                   << "High: " << data.high_price << " | "
                   << "Low: " << data.low_price << " | "
                   << "Close: " << data.close_price << " | "
                   << "Volume: " << data.volume << " | "
                   << "OI: " << data.oi << "\n";
        m_txt_file.flush();
    }
    
    // Write to CSV file
    if (m_csv_file.is_open()) {
        m_csv_file << data.timestamp << ","
                   << data.symbol << ","
                   << data.exchange << ","
                   << data.last_price << ","
                   << data.open_price << ","
                   << data.high_price << ","
                   << data.low_price << ","
                   << data.close_price << ","
                   << data.volume << ","
                   << data.oi << ","
                   << data.mode << "\n";
        m_csv_file.flush();
    }
}

void AngelOneWebSocket::initializeCsvFile() {
    if (m_csv_file.is_open()) {
        // Write CSV header
        m_csv_file << "timestamp,symbol,exchange,last_price,open_price,high_price,low_price,"
                   << "close_price,volume,oi,mode\n";
        m_csv_file.flush();
        m_csv_initialized = true;
    }
}

std::string AngelOneWebSocket::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string AngelOneWebSocket::generateAuthHeader() {
    // AngelOne typically uses the access token directly
    // Format may vary, adjust based on current API documentation
    return "Bearer " + m_access_token;
}
