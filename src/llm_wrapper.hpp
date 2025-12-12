#ifndef LLM_WRAPPER_HPP
#define LLM_WRAPPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Forward declarations for llm.c types
// These will be defined based on actual llm.c API
struct llm_model;
struct llm_context;

namespace llm_wrapper {

/**
 * LLM Wrapper class that provides C++ interface to llm.c
 */
class LLMWrapper {
public:
    LLMWrapper();
    ~LLMWrapper();

    // Disable copy constructor and assignment
    LLMWrapper(const LLMWrapper&) = delete;
    LLMWrapper& operator=(const LLMWrapper&) = delete;

    /**
     * Load a model from file
     * @param model_path Path to the model file
     * @return true if successful, false otherwise
     */
    bool loadModel(const std::string& model_path);

    /**
     * Check if model is loaded
     */
    bool isLoaded() const { return model_loaded_; }

    /**
     * Generate text from prompt
     * @param prompt Input prompt
     * @param max_tokens Maximum tokens to generate
     * @param temperature Sampling temperature (0.0-2.0)
     * @param top_k Top-k sampling
     * @param top_p Top-p (nucleus) sampling
     * @return Generated text
     */
    std::string generate(const std::string& prompt, 
                        int max_tokens = 256,
                        float temperature = 0.7f,
                        int top_k = 40,
                        float top_p = 0.9f);

    /**
     * Generate text with streaming callback
     * @param prompt Input prompt
     * @param callback Function called for each generated token
     * @param max_tokens Maximum tokens to generate
     * @param temperature Sampling temperature
     * @param top_k Top-k sampling
     * @param top_p Top-p sampling
     * @return Full generated text
     */
    std::string generateStream(const std::string& prompt,
                              std::function<void(const std::string&)> callback,
                              int max_tokens = 256,
                              float temperature = 0.7f,
                              int top_k = 40,
                              float top_p = 0.9f);

    /**
     * Get the context window size
     */
    size_t getContextSize() const { return context_size_; }

    /**
     * Set context window size
     */
    void setContextSize(size_t size) { context_size_ = size; }

    /**
     * Reset the context
     */
    void resetContext();

    /**
     * Get model information
     */
    std::string getModelInfo() const;

private:
    bool model_loaded_;
    size_t context_size_;
    
    // llm.c model and context pointers
    // Using void* to avoid including llm.c headers directly
    // Actual implementation will cast to proper types
    void* model_ptr_;
    void* context_ptr_;

    /**
     * Initialize llm.c context
     */
    bool initializeContext();

    /**
     * Cleanup resources
     */
    void cleanup();
};

} // namespace llm_wrapper

#endif // LLM_WRAPPER_HPP

