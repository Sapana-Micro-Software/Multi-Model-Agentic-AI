#include "llm_wrapper.hpp"
#include "verbose.hpp"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <regex>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>

#ifdef OLLAMA_SUPPORT
#include <curl/curl.h>
#endif

// Try to include llm.c headers if available
// If not available, we'll provide a stub implementation
#ifdef LLM_C_AVAILABLE
#include "llm.h"
#else
// Stub definitions for when llm.c is not available
// These will be replaced when llm.c is properly integrated
struct llm_model {};
struct llm_context {};
#endif

namespace llm_wrapper {

// Static variables for thread-safe CURL initialization
static std::mutex curl_init_mutex;
static int curl_init_count = 0;
static bool curl_initialized = false;

// Static callback function for CURL write operations
// This must be a static function (not a lambda) to ensure thread safety
// and prevent segfaults when CURL calls it from different threads
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (!userp || !contents) return 0;
    std::string* data = static_cast<std::string*>(userp);
    if (!data) return 0;
    size_t total_size = size * nmemb;
    try {
        data->append(static_cast<const char*>(contents), total_size);
    } catch (...) {
        return 0; // Return 0 on exception to abort transfer
    }
    return total_size;
}

// Semaphore-like structure for limiting concurrent CURL operations
// CURL handles are thread-safe, but we limit concurrent requests to prevent overwhelming Ollama
class CurlSemaphore {
public:
    CurlSemaphore(size_t max_concurrent) 
        : max_concurrent_(max_concurrent), current_count_(0) {}
    
    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return current_count_ < max_concurrent_; });
        current_count_++;
    }
    
    void release() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_count_ > 0) {
            current_count_--;
            condition_.notify_one();
        }
    }
    
private:
    size_t max_concurrent_;
    size_t current_count_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

// Semaphore to limit concurrent Ollama requests (max 4 concurrent by default)
// This allows parallelism while preventing overwhelming the Ollama server
static CurlSemaphore curl_operation_semaphore(4);  // Allow up to 4 concurrent requests

LLMWrapper::LLMWrapper()
    : model_loaded_(false)
    , context_size_(2048)
    , backend_type_(BackendType::STUB)
    , model_ptr_(nullptr)
    , context_ptr_(nullptr)
    , ollama_url_("http://localhost:11434")
{
#ifdef OLLAMA_SUPPORT
    std::lock_guard<std::mutex> lock(curl_init_mutex);
    curl_init_count++;
    if (!curl_initialized) {
        CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (res == CURLE_OK) {
            curl_initialized = true;
            verbose::log("CURL global initialization successful", "CURL Init");
        } else {
            verbose::log("CURL global initialization failed: " + std::to_string(res), "CURL Init");
        }
    }
#endif
}

LLMWrapper::~LLMWrapper() {
    cleanup();
#ifdef OLLAMA_SUPPORT
    std::lock_guard<std::mutex> lock(curl_init_mutex);
    curl_init_count--;
    if (curl_init_count == 0 && curl_initialized) {
        curl_global_cleanup();
        curl_initialized = false;
        verbose::log("CURL global cleanup completed", "CURL Cleanup");
    }
#endif
}

void LLMWrapper::cleanup() {
    if (context_ptr_) {
        // Cleanup context
        // llm_free_context((llm_context*)context_ptr_);
        context_ptr_ = nullptr;
    }
    if (model_ptr_) {
        // Cleanup model
        // llm_free_model((llm_model*)model_ptr_);
        model_ptr_ = nullptr;
    }
    model_loaded_ = false;
}

bool LLMWrapper::loadModel(const std::string& model_path, const std::string& ollama_url) {
    verbose::logPhase("MODEL LOADING: " + model_path);
    verbose::logStep("Model Loading", 1, "Checking if model already loaded");
    
    if (model_loaded_) {
        verbose::log("Cleaning up existing model", "Model Loading");
        cleanup();
    }
    
    ollama_url_ = ollama_url;

    verbose::logStep("Model Loading", 2, "Detecting model type");
    // Check if this is an Ollama model
    if (isOllamaModel(model_path)) {
        verbose::log("Detected Ollama model", "Model Loading");
        backend_type_ = BackendType::OLLAMA;
        ollama_model_name_ = extractOllamaModelName(model_path);
        verbose::log("Ollama model name: " + ollama_model_name_, "Model Loading");
        
        // Don't verify Ollama connection here - it might block
        // The actual model will be verified on first generate() call
#ifdef OLLAMA_SUPPORT
        verbose::log("Ollama support enabled (will verify on first use)", "Model Loading");
        // Just mark as loaded - verification happens on first use
        // This prevents blocking during agent initialization
#else
        verbose::log("Ollama support not compiled, using stub mode", "Model Loading");
        std::cerr << "Warning: Ollama support not compiled. Using stub mode." << std::endl;
#endif
        verbose::logStep("Model Loading", 3, "Marking Ollama model as loaded");
        model_loaded_ = true;
        verbose::log("Ollama model loading completed", "Model Loading");
        return true;
    }

    verbose::log("Detected file-based model", "Model Loading");
    // File-based model (llm.c)
    backend_type_ = BackendType::LLM_C;
    
    verbose::logStep("Model Loading", 3, "Checking if model file exists: " + model_path);
    // Check if file exists (but allow stub mode if file doesn't exist)
    std::ifstream file(model_path);
    bool file_exists = file.good();
    file.close();
    verbose::log("File exists: " + std::string(file_exists ? "yes" : "no"), "Model Loading");
    
    // In stub mode (LLM_C_AVAILABLE not defined), allow missing files
    // This enables testing without actual model files
#ifndef LLM_C_AVAILABLE
    verbose::log("Using stub mode (LLM_C not available)", "Model Loading");
    // Stub mode: allow missing model files for testing
    // Just continue with stub implementation
    backend_type_ = BackendType::STUB;
#else
    if (!file_exists) {
        verbose::log("Model file not found and LLM_C required - loading failed", "Model Loading");
        return false;
    }
#endif

#ifdef LLM_C_AVAILABLE
    verbose::logStep("Model Loading", 4, "Loading model using llm.c API");
    // Load model using llm.c API
    // model_ptr_ = llm_load_model(model_path.c_str());
    // if (!model_ptr_) {
    //     return false;
    // }
    
    verbose::logStep("Model Loading", 5, "Initializing context");
    // Initialize context
    if (!initializeContext()) {
        verbose::log("Context initialization failed", "Model Loading");
        cleanup();
        return false;
    }
    verbose::log("Context initialized successfully", "Model Loading");
#else
    verbose::logStep("Model Loading", 4, "Using stub implementation");
    // Stub implementation - just mark as loaded
    // In real implementation, this would load the actual model
    model_ptr_ = reinterpret_cast<void*>(1); // Placeholder
    verbose::log("Stub model pointer set", "Model Loading");
#endif

    verbose::logStep("Model Loading", 6, "Marking model as loaded");
    model_loaded_ = true;
    verbose::log("Model loading completed successfully", "Model Loading");
    return true;
}

bool LLMWrapper::initializeContext() {
#ifdef LLM_C_AVAILABLE
    // context_ptr_ = llm_new_context((llm_model*)model_ptr_, context_size_);
    // return context_ptr_ != nullptr;
    return true;
#else
    context_ptr_ = reinterpret_cast<void*>(1); // Placeholder
    return true;
#endif
}

void LLMWrapper::resetContext() {
    if (context_ptr_) {
        // llm_reset_context((llm_context*)context_ptr_);
    }
}

std::string LLMWrapper::generate(const std::string& prompt,
                                 int max_tokens,
                                 float temperature,
                                 int top_k,
                                 float top_p) {
    std::cout << "[DEBUG INFERENCE] LLMWrapper::generate: Entry" << std::endl;
    std::cout << "[DEBUG INFERENCE]   Prompt length: " << prompt.length() << " chars" << std::endl;
    std::cout << "[DEBUG INFERENCE]   Max tokens: " << max_tokens << ", temp: " << temperature << std::endl;
    std::cout.flush();
    
    verbose::logStep("LLM Generate", 1, "Validating model loaded");
    if (!model_loaded_) {
        std::cout << "[DEBUG INFERENCE] ERROR: Model not loaded!" << std::endl;
        std::cout.flush();
        verbose::log("Model not loaded - error", "LLM Generate");
        throw std::runtime_error("Model not loaded");
    }

    verbose::log("Prompt length: " + std::to_string(prompt.length()) + " chars, max_tokens: " + std::to_string(max_tokens), "LLM Generate");
    
    std::cout << "[DEBUG INFERENCE] Backend type: " << (backend_type_ == BackendType::OLLAMA ? "OLLAMA" : 
                                                         backend_type_ == BackendType::LLM_C ? "LLM_C" : "STUB") << std::endl;
    std::cout.flush();
    
    // Route to appropriate backend
    if (backend_type_ == BackendType::OLLAMA) {
        std::cout << "[DEBUG INFERENCE] Routing to Ollama backend" << std::endl;
        std::cout.flush();
        verbose::logStep("LLM Generate", 2, "Using Ollama backend");
        std::string result = generateOllama(prompt, max_tokens, temperature, top_k, top_p);
        std::cout << "[DEBUG INFERENCE] Ollama returned " << result.length() << " chars" << std::endl;
        std::cout.flush();
        return result;
    }
    
    std::cout << "[DEBUG INFERENCE] Using " << (backend_type_ == BackendType::LLM_C ? "llm.c" : "stub") << " backend" << std::endl;
    std::cout.flush();
    verbose::logStep("LLM Generate", 2, "Using " + std::string(backend_type_ == BackendType::LLM_C ? "llm.c" : "stub") + " backend");

#ifdef LLM_C_AVAILABLE
    // Use llm.c API to generate
    // std::string result;
    // char* output = llm_generate((llm_context*)context_ptr_,
    //                             prompt.c_str(),
    //                             max_tokens,
    //                             temperature,
    //                             top_k,
    //                             top_p);
    // if (output) {
    //     result = output;
    //     free(output);
    // }
    // return result;
    
    // Stub: return modified prompt
    return "[Generated from: " + prompt + "]";
#else
    // Stub implementation for testing
    return "[LLM Generated Response for: " + prompt + "]";
#endif
}

std::string LLMWrapper::generateStream(const std::string& prompt,
                                       std::function<void(const std::string&)> callback,
                                       int max_tokens,
                                       float temperature,
                                       int top_k,
                                       float top_p) {
    if (!model_loaded_) {
        throw std::runtime_error("Model not loaded");
    }

    std::string full_output;
    
#ifdef LLM_C_AVAILABLE
    // Stream generation using llm.c API
    // llm_generate_stream((llm_context*)context_ptr_,
    //                     prompt.c_str(),
    //                     max_tokens,
    //                     temperature,
    //                     top_k,
    //                     top_p,
    //                     [](const char* token, void* user_data) {
    //                         auto* output = static_cast<std::string*>(user_data);
    //                         *output += token;
    //                         auto* callback = static_cast<std::function<void(const std::string&)>*>(
    //                             static_cast<void**>(user_data)[1]);
    //                         (*callback)(token);
    //                     },
    //                     &full_output);
#else
    // Stub: simulate streaming
    std::string stub_output = "[Streaming response for: " + prompt + "]";
    for (size_t i = 0; i < stub_output.length(); ++i) {
        std::string token(1, stub_output[i]);
        full_output += token;
        if (callback) {
            callback(token);
        }
    }
#endif

    return full_output;
}

std::string LLMWrapper::getModelInfo() const {
    if (!model_loaded_) {
        return "Model not loaded";
    }

    std::ostringstream oss;
    oss << "Model loaded: Yes\n";
    oss << "Backend: ";
    switch (backend_type_) {
        case BackendType::OLLAMA:
            oss << "Ollama (" << ollama_model_name_ << ")\n";
            oss << "Ollama URL: " << ollama_url_ << "\n";
            break;
        case BackendType::LLM_C:
            oss << "llm.c\n";
            break;
        case BackendType::STUB:
            oss << "Stub\n";
            break;
    }
    oss << "Context size: " << context_size_ << "\n";
    return oss.str();
}

bool LLMWrapper::isOllamaModel(const std::string& model_path) const {
    // Check if it starts with "ollama:" or doesn't look like a file path
    if (model_path.find("ollama:") == 0) {
        return true;
    }
    
    // If it doesn't contain path separators and doesn't end with common model extensions,
    // assume it's an Ollama model name
    if (model_path.find("/") == std::string::npos && 
        model_path.find("\\") == std::string::npos &&
        model_path.find(".") == std::string::npos) {
        return true;
    }
    
    return false;
}

std::string LLMWrapper::extractOllamaModelName(const std::string& model_path) const {
    if (model_path.find("ollama:") == 0) {
        return model_path.substr(7); // Remove "ollama:" prefix
    }
    return model_path;
}

std::string LLMWrapper::generateOllama(const std::string& prompt,
                                       int max_tokens,
                                       float temperature,
                                       int top_k,
                                       float top_p) {
    std::cout << "[DEBUG INFERENCE] generateOllama: Entry" << std::endl;
    std::cout << "[DEBUG INFERENCE]   Model: " << ollama_model_name_ << std::endl;
    std::cout << "[DEBUG INFERENCE]   URL: " << ollama_url_ << std::endl;
    std::cout << "[DEBUG INFERENCE]   Prompt preview: " << prompt.substr(0, 100) << "..." << std::endl;
    std::cout.flush();
    
    verbose::logPhase("OLLAMA GENERATION");
    verbose::log("Model: " + ollama_model_name_ + ", URL: " + ollama_url_, "Ollama");
    
#ifdef OLLAMA_SUPPORT
    verbose::logStep("Ollama", 1, "Building API URL");
    std::string url = ollama_url_ + "/api/generate";
    verbose::log("API URL: " + url, "Ollama");
    
    verbose::logStep("Ollama", 2, "Escaping prompt for JSON");
    // Escape JSON string (simple escaping for prompt)
    std::string escaped_prompt;
    for (char c : prompt) {
        if (c == '"') {
            escaped_prompt += "\\\"";
        } else if (c == '\\') {
            escaped_prompt += "\\\\";
        } else if (c == '\n') {
            escaped_prompt += "\\n";
        } else if (c == '\r') {
            escaped_prompt += "\\r";
        } else if (c == '\t') {
            escaped_prompt += "\\t";
        } else {
            escaped_prompt += c;
        }
    }
    verbose::log("Prompt escaped (" + std::to_string(escaped_prompt.length()) + " chars)", "Ollama");
    
    verbose::logStep("Ollama", 3, "Building JSON request");
    // Build JSON request
    std::ostringstream json;
    json << "{"
         << "\"model\": \"" << ollama_model_name_ << "\","
         << "\"prompt\": \"" << escaped_prompt << "\","
         << "\"stream\": false,"
         << "\"options\": {"
         << "\"num_predict\": " << max_tokens << ","
         << "\"temperature\": " << temperature << ","
         << "\"top_k\": " << top_k << ","
         << "\"top_p\": " << top_p
         << "}"
         << "}";
    
    verbose::log("JSON request built (" + std::to_string(json.str().length()) + " chars)", "Ollama");
    verbose::logStep("Ollama", 4, "Checking Ollama server connection");
    
    // Check if Ollama is reachable before making the request
    if (!checkOllamaConnection(ollama_url_)) {
        std::cerr << "[ERROR] Cannot connect to Ollama server at " << ollama_url_ << std::endl;
        std::cerr << "[ERROR] Please ensure Ollama is running. Start it with: ollama serve" << std::endl;
        std::cerr << "[ERROR] Or verify connection with: curl " << ollama_url_ << "/api/tags" << std::endl;
        std::cerr.flush();
        verbose::log("Ollama connection check failed", "Ollama");
        return "[Error: Ollama server not reachable at " + ollama_url_ + ". Please start Ollama with 'ollama serve']";
    }
    
    verbose::log("Ollama server is reachable", "Ollama");
    verbose::logStep("Ollama", 5, "Checking if model is available");
    
    // Quick synchronous check (fast, non-blocking for already-loaded models)
    // Skip the check if it might cause issues - the HTTP POST will handle model loading anyway
    // Use a signal-safe approach: catch all exceptions and proceed if check fails
    bool model_check_success = false;
    try {
        std::cout << "[DEBUG] About to call checkOllamaModelAvailable..." << std::endl;
        std::cout.flush();
        bool model_available = checkOllamaModelAvailable(ollama_url_, ollama_model_name_);
        std::cout << "[DEBUG] checkOllamaModelAvailable returned: " << (model_available ? "true" : "false") << std::endl;
        std::cout.flush();
        model_check_success = true;
        
        if (!model_available) {
            std::cout << "[WARNING] Model '" << ollama_model_name_ << "' may need to be loaded by Ollama." << std::endl;
            std::cout << "[WARNING] This can take several minutes on first use. Please wait..." << std::endl;
            std::cout << "[WARNING] To preload the model, run: ollama pull " << ollama_model_name_ << std::endl;
            std::cout.flush();
            verbose::log("Model availability check: model may need loading", "Ollama");
        } else {
            verbose::log("Model is available in Ollama", "Ollama");
        }
    } catch (const std::exception& e) {
        // If check fails, just proceed - HTTP POST will handle model loading
        std::cout << "[INFO] Model check exception: " << e.what() << std::endl;
        std::cout << "[INFO] Proceeding anyway - HTTP POST will handle model loading" << std::endl;
        std::cout.flush();
        verbose::log("Model availability check failed with exception, proceeding anyway", "Ollama");
    } catch (...) {
        // Catch any other exceptions (including segfaults that might be caught by signal handlers)
        std::cout << "[INFO] Model check failed with unknown error, proceeding anyway..." << std::endl;
        std::cout << "[INFO] If this is the first use, Ollama will load the model (may take 2-5 minutes)" << std::endl;
        std::cout.flush();
        verbose::log("Model availability check failed, proceeding anyway", "Ollama");
    }
    
    if (!model_check_success) {
        std::cout << "[INFO] Skipping model availability check due to errors" << std::endl;
        std::cout.flush();
    }
    
    verbose::logStep("Ollama", 6, "Sending HTTP POST request to Ollama");
    std::cout << "[DEBUG INFERENCE] About to call httpPost - this may take time..." << std::endl;
    std::cout << "[DEBUG INFERENCE] If model is loading for the first time, this can take 2-5 minutes..." << std::endl;
    std::cout.flush();
    std::string response = httpPost(url, json.str());
    std::cout << "[DEBUG INFERENCE] httpPost returned " << response.length() << " chars" << std::endl;
    std::cout.flush();
    verbose::log("HTTP response received (" + std::to_string(response.length()) + " chars)", "Ollama");
    
    verbose::logStep("Ollama", 7, "Parsing JSON response");
    // Parse JSON response (simple extraction)
    // Ollama returns: {"response": "text here", ...}
    size_t response_start = response.find("\"response\":\"");
    if (response_start != std::string::npos) {
        response_start += 12; // Skip "response":"
        size_t response_end = response.find("\"", response_start);
        if (response_end != std::string::npos) {
            std::string result = response.substr(response_start, response_end - response_start);
            // Unescape JSON strings
            std::string unescaped;
            for (size_t i = 0; i < result.length(); ++i) {
                if (result[i] == '\\' && i + 1 < result.length()) {
                    if (result[i+1] == 'n') {
                        unescaped += '\n';
                        ++i;
                        continue;
                    } else if (result[i+1] == '\\') {
                        unescaped += '\\';
                        ++i;
                        continue;
                    }
                }
                unescaped += result[i];
            }
            verbose::log("Response extracted (" + std::to_string(unescaped.length()) + " chars)", "Ollama");
            return unescaped;
        }
    }
    
    verbose::log("Response parsing: using fallback", "Ollama");
    // Fallback: return raw response or error message
    if (response.empty()) {
        verbose::log("Empty response - error", "Ollama");
        return "[Ollama Error: No response received]";
    }
    verbose::log("Using raw response as fallback", "Ollama");
    return "[Ollama Response: " + response + "]";
#else
    verbose::log("Ollama not supported (no curl)", "Ollama");
    return "[Ollama not supported - compiled without curl]";
#endif
}

std::string LLMWrapper::httpPost(const std::string& url, const std::string& json_data) const {
#ifdef OLLAMA_SUPPORT
    // Acquire semaphore permit for concurrent request (allows up to 4 concurrent requests)
    // CURL handles are thread-safe, so we can allow concurrent requests
    // The semaphore prevents overwhelming Ollama with too many simultaneous requests
    curl_operation_semaphore.acquire();
    
    // RAII guard to ensure semaphore is released even on exception
    struct SemaphoreGuard {
        CurlSemaphore& sem;
        SemaphoreGuard(CurlSemaphore& s) : sem(s) {}
        ~SemaphoreGuard() { sem.release(); }
    } guard(curl_operation_semaphore);
    
    verbose::log("Initializing CURL", "HTTP");
    CURL* curl = curl_easy_init();
    if (!curl) {
        verbose::log("CURL initialization failed", "HTTP");
        return "";
    }
    
    std::string response_data;
    auto start_time = std::chrono::steady_clock::now();
    
    verbose::log("Setting up HTTP headers", "HTTP");
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    verbose::log("Configuring CURL options", "HTTP");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // CRITICAL FIX: Use CURLOPT_COPYPOSTFIELDS instead of CURLOPT_POSTFIELDS
    // This ensures CURL makes its own copy of the data, preventing segfaults
    // if the original string goes out of scope or is modified
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
    
    // Add connection timeout to prevent indefinite hangs
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);  // 10 second connection timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);         // 5 minute total timeout
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);   // Abort if speed < 1 byte/sec
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);   // For 30 seconds
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // Use static function instead of lambda for thread safety
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    
    // Disable progress callback to avoid segfault issues
    // Progress can be tracked via response data size instead
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    
    verbose::log("Executing HTTP POST request to Ollama (loading model...)", "HTTP");
    std::cout << "[DEBUG INFERENCE] Connecting to: " << url << std::endl;
    std::cout << "[DEBUG INFERENCE] Connection timeout: 10s, Total timeout: 300s" << std::endl;
    std::cout << "[DEBUG INFERENCE] NOTE: If model is loading for the first time, this can take 2-5 minutes." << std::endl;
    std::cout << "[DEBUG INFERENCE] Calling curl_easy_perform (thread-safe)..." << std::endl;
    std::cout << "[DEBUG INFERENCE] Waiting for Ollama response (this may take time if model is loading)..." << std::endl;
    std::cout.flush();
    
    // Perform the request - this will block until Ollama responds
    // If the model is loading, this can take several minutes
    CURLcode res = curl_easy_perform(curl);
    
    // Log immediately after curl_easy_perform returns
    std::cout << "[DEBUG INFERENCE] curl_easy_perform returned: " << res << std::endl;
    std::cout.flush();
    
    // Clean up CURL resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        const char* error_msg = curl_easy_strerror(res);
        std::cout << "[DEBUG INFERENCE] CURL error: " << res << " (" << (error_msg ? error_msg : "unknown") << ")" << std::endl;
        std::cout.flush();
        verbose::log("HTTP request failed with CURL error: " + std::to_string(res) + " (" + (error_msg ? std::string(error_msg) : "unknown") + ")", "HTTP");
        
        // Provide helpful error messages for common issues
        if (res == CURLE_COULDNT_CONNECT || res == CURLE_COULDNT_RESOLVE_HOST) {
            std::cerr << "[ERROR] Cannot connect to Ollama server. Is it running?" << std::endl;
            std::cerr << "[ERROR] Start Ollama with: ollama serve" << std::endl;
            std::cerr.flush();
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            std::cerr << "[ERROR] Connection to Ollama timed out. The server may be overloaded or unreachable." << std::endl;
            std::cerr.flush();
        }
        return "";
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();
    std::cout << "[DEBUG INFERENCE] HTTP request completed: " << elapsed << "ms, " 
              << response_data.length() << " bytes" << std::endl;
    std::cout.flush();
    verbose::log("HTTP request completed successfully (" + std::to_string(elapsed) + "ms, " + 
                std::to_string(response_data.length()) + " bytes)", "HTTP");
    return response_data;
#else
    verbose::log("HTTP not supported (no curl)", "HTTP");
    return "";
#endif
}

bool LLMWrapper::checkOllamaConnection(const std::string& url) const {
#ifdef OLLAMA_SUPPORT
    // Acquire semaphore permit for connection check
    curl_operation_semaphore.acquire();
    
    // RAII guard to ensure semaphore is released
    struct SemaphoreGuard {
        CurlSemaphore& sem;
        SemaphoreGuard(CurlSemaphore& s) : sem(s) {}
        ~SemaphoreGuard() { sem.release(); }
    } guard(curl_operation_semaphore);
    
    verbose::log("Checking Ollama connection: " + url, "HTTP");
    CURL* curl = curl_easy_init();
    if (!curl) {
        verbose::log("CURL initialization failed for connection check", "HTTP");
        return false;
    }
    
    std::string test_url = url + "/api/tags";
    curl_easy_setopt(curl, CURLOPT_URL, test_url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);  // 5 second connection timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);         // 5 second total timeout
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);          // HEAD request only (faster)
    
    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    }
    curl_easy_cleanup(curl);
    
    bool connected = (res == CURLE_OK && response_code == 200);
    if (connected) {
        verbose::log("Ollama connection check: SUCCESS", "HTTP");
    } else {
        verbose::log("Ollama connection check: FAILED (CURL error: " + std::to_string(res) + ", HTTP code: " + std::to_string(response_code) + ")", "HTTP");
    }
    return connected;
#else
    verbose::log("HTTP not supported (no curl) - connection check skipped", "HTTP");
    return false;
#endif
}

bool LLMWrapper::checkOllamaModelAvailable(const std::string& url, const std::string& model_name) const {
#ifdef OLLAMA_SUPPORT
    // Use std::cout directly for critical messages to avoid verbose::log issues
    std::cout << "[DEBUG] checkOllamaModelAvailable: Entry - model=" << model_name << std::endl;
    std::cout.flush();
    
    try {
        // Acquire semaphore permit for model check (prevents concurrent CURL access)
        std::cout << "[DEBUG] checkOllamaModelAvailable: Acquiring semaphore..." << std::endl;
        std::cout.flush();
        curl_operation_semaphore.acquire();
        std::cout << "[DEBUG] checkOllamaModelAvailable: Semaphore acquired" << std::endl;
        std::cout.flush();
        
        // RAII guard to ensure semaphore is released
        struct SemaphoreGuard {
            CurlSemaphore& sem;
            bool released;
            SemaphoreGuard(CurlSemaphore& s) : sem(s), released(false) {}
            ~SemaphoreGuard() { 
                if (!released) {
                    try {
                        sem.release(); 
                    } catch (...) {
                        // Ignore exceptions in destructor
                    }
                }
            }
            void release() {
                if (!released) {
                    try {
                        sem.release();
                        released = true;
                    } catch (...) {
                        // Ignore exceptions
                    }
                }
            }
        } guard(curl_operation_semaphore);
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: About to log verbose message" << std::endl;
        std::cout.flush();
        verbose::log("Checking if model '" + model_name + "' is available in Ollama", "HTTP");
        std::cout << "[DEBUG] checkOllamaModelAvailable: Verbose log completed" << std::endl;
        std::cout.flush();
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: Initializing CURL..." << std::endl;
        std::cout.flush();
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cout << "[DEBUG] checkOllamaModelAvailable: CURL init failed" << std::endl;
            std::cout.flush();
            verbose::log("CURL initialization failed for model check", "HTTP");
            guard.release();
            return false;
        }
        std::cout << "[DEBUG] checkOllamaModelAvailable: CURL initialized successfully" << std::endl;
        std::cout.flush();
        
        std::string test_url = url + "/api/tags";
        std::string response_data;
        response_data.reserve(4096); // Pre-allocate to avoid reallocations
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: Setting up CURL options for " << test_url << std::endl;
        std::cout.flush();
        
        // Set up CURL options with short timeouts for quick check
        curl_easy_setopt(curl, CURLOPT_URL, test_url.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);  // 3 second connection timeout (faster)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);         // 5 second total timeout (faster)
        
        // Use static function instead of lambda for thread safety
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: Executing CURL request..." << std::endl;
        std::cout.flush();
        CURLcode res = curl_easy_perform(curl);
        std::cout << "[DEBUG] checkOllamaModelAvailable: CURL request completed, result=" << res << std::endl;
        std::cout.flush();
        
        long response_code = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        }
        
        // Clean up CURL handle immediately
        curl_easy_cleanup(curl);
        curl = nullptr; // Prevent use after cleanup
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: CURL cleaned up, releasing semaphore" << std::endl;
        std::cout.flush();
        guard.release(); // Release semaphore before any further operations
        
        if (res != CURLE_OK || response_code != 200) {
            std::cout << "[DEBUG] checkOllamaModelAvailable: Request failed (CURL=" << res << ", HTTP=" << response_code << ")" << std::endl;
            std::cout.flush();
            verbose::log("Failed to get model list from Ollama (CURL error: " + std::to_string(res) + ", HTTP: " + std::to_string(response_code) + ")", "HTTP");
            return false; // Assume model not available if we can't check
        }
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: Parsing response (" << response_data.length() << " bytes)" << std::endl;
        std::cout.flush();
        
        // Parse JSON response to check if model is in the list
        // Ollama returns: {"models": [{"name": "model1", ...}, {"name": "model2", ...}]}
        // Simple string search for the model name (not perfect JSON parsing, but sufficient)
        bool found = false;
        if (!response_data.empty()) {
            std::string search_pattern = "\"name\":\"" + model_name + "\"";
            found = response_data.find(search_pattern) != std::string::npos;
            
            // Also check for model with tag (e.g., "mistral:latest" or "tinyllama:latest")
            if (!found) {
                search_pattern = "\"name\":\"" + model_name + ":";
                found = response_data.find(search_pattern) != std::string::npos;
            }
        }
        
        std::cout << "[DEBUG] checkOllamaModelAvailable: Model found=" << (found ? "true" : "false") << std::endl;
        std::cout.flush();
        
        if (found) {
            verbose::log("Model '" + model_name + "' is available in Ollama", "HTTP");
        } else {
            verbose::log("Model '" + model_name + "' not found in Ollama model list", "HTTP");
        }
        return found;
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] checkOllamaModelAvailable: Exception caught: " << e.what() << std::endl;
        std::cout.flush();
        verbose::log("Exception in checkOllamaModelAvailable: " + std::string(e.what()), "HTTP");
        return false;
    } catch (...) {
        std::cout << "[DEBUG] checkOllamaModelAvailable: Unknown exception caught" << std::endl;
        std::cout.flush();
        verbose::log("Unknown exception in checkOllamaModelAvailable", "HTTP");
        return false;
    }
#else
    verbose::log("HTTP not supported (no curl) - model check skipped", "HTTP");
    return false;
#endif
}

std::future<bool> LLMWrapper::checkOllamaModelAvailableAsync(const std::string& url, const std::string& model_name) const {
    // Note: This function is kept for API compatibility but is not recommended
    // Creating a temporary LLMWrapper causes CURL initialization conflicts
    // Use the synchronous checkOllamaModelAvailable instead
    std::promise<bool> promise;
    try {
        // Just return false immediately to avoid CURL conflicts
        // The synchronous check is fast enough and safer
        promise.set_value(false);
    } catch (...) {
        promise.set_value(false);
    }
    return promise.get_future();
}

} // namespace llm_wrapper

