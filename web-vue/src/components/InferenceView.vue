<template>
  <div class="inference-view">
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-primary text-white">
            <i class="bi bi-chat-dots me-2"></i>模型推理
          </div>
          <div class="card-body">
            <form @submit.prevent="runInference">
              <!-- 模型路径 -->
              <div class="mb-3">
                <label for="model_path" class="form-label">模型路径</label>
                <input 
                  type="text" 
                  class="form-control" 
                  id="model_path" 
                  v-model="inferenceData.model_path"
                  placeholder="输入模型路径，例如：./output/task_name/lora"
                  required
                >
                <div class="form-text">训练好的模型路径或检查点</div>
              </div>
              
              <!-- 输入提示 -->
              <div class="mb-3">
                <label for="prompt" class="form-label">输入内容</label>
                <textarea 
                  class="form-control" 
                  id="prompt" 
                  v-model="inferenceData.prompt"
                  rows="5"
                  placeholder="请输入您想问的问题..."
                  required
                ></textarea>
              </div>
              
              <!-- 参数设置 -->
              <div class="row mb-3">
                <div class="col-md-4">
                  <label for="max_new_tokens" class="form-label">最大生成长度</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="max_new_tokens" 
                    v-model.number="inferenceData.max_new_tokens"
                    min="1"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="temperature" class="form-label">温度</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="temperature" 
                    v-model.number="inferenceData.temperature"
                    step="0.1"
                    min="0.1"
                    max="2.0"
                    required
                  >
                  <div class="form-text">值越高，回复越随机</div>
                </div>
                <div class="col-md-4">
                  <label for="top_p" class="form-label">Top P</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="top_p" 
                    v-model.number="inferenceData.top_p"
                    step="0.01"
                    min="0"
                    max="1"
                    required
                  >
                </div>
              </div>
              
              <!-- 提交按钮 -->
              <div class="d-grid gap-2 col-6 mx-auto">
                <button 
                  type="submit" 
                  class="btn btn-primary"
                  :disabled="inferencing"
                >
                  <i class="bi bi-send me-2"></i>
                  {{ inferencing ? '生成中...' : '开始推理' }}
                </button>
              </div>
            </form>
          </div>
        </div>
      </div>
    </div>
    
    <!-- 推理结果 -->
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-success text-white">
            <i class="bi bi-reply-fill me-2"></i>推理结果
          </div>
          <div class="card-body">
            <div v-if="inferencing" class="loading-spinner">
              <div class="spinner-border text-primary" role="status">
                <span class="visually-hidden">Loading...</span>
              </div>
              <p class="ms-2">模型思考中，请稍候...</p>
            </div>
            <div v-else-if="inferenceError" class="alert alert-danger">
              <i class="bi bi-exclamation-circle me-2"></i>{{ inferenceError }}
            </div>
            <div v-else-if="inferenceResult" class="inference-result">
              <div class="alert alert-light border">
                <pre class="mb-0 response-text">{{ inferenceResult }}</pre>
              </div>
            </div>
            <div v-else class="text-center text-muted">
              <i class="bi bi-chat-square-text me-2"></i>在上方输入内容后点击"开始推理"按钮
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'InferenceView',
  data() {
    return {
      inferenceData: {
        model_path: './output/task_name/lora',
        prompt: '',
        max_new_tokens: 2048,
        temperature: 0.7,
        top_p: 0.9
      },
      inferencing: false,
      inferenceResult: null,
      inferenceError: null
    }
  },
  methods: {
    runInference() {
      this.inferencing = true
      this.inferenceError = null
      this.inferenceResult = null
      
      // 调用后端API
      fetch('/api/inference', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(this.inferenceData)
      })
      .then(response => {
        if (!response.ok) {
          throw new Error('推理请求失败');
        }
        return response.json();
      })
      .then(data => {
        if (!data.success) {
          throw new Error(data.message || '推理请求失败');
        }
        
        // 开始轮询结果
        this.pollInferenceResult(data.output_file);
      })
      .catch(error => {
        this.inferenceError = `推理失败: ${error.message}`;
        this.inferencing = false;
      });
    },
    
    // 轮询推理结果
    pollInferenceResult(outputFile) {
      const encodedFile = encodeURIComponent(outputFile);
      const pollInterval = setInterval(() => {
        fetch(`/api/inference/result?file=${encodedFile}`)
          .then(response => {
            if (!response.ok) {
              throw new Error('获取推理结果失败');
            }
            return response.json();
          })
          .then(data => {
            if (data.success) {
              // 清除轮询
              clearInterval(pollInterval);
              
              // 显示结果
              this.inferenceResult = data.result;
              this.inferencing = false;
            }
          })
          .catch(error => {
            // 清除轮询
            clearInterval(pollInterval);
            
            this.inferenceError = `获取推理结果失败: ${error.message}`;
            this.inferencing = false;
          });
      }, 3000); // 每3秒轮询一次
      
      // 设置超时，避免无限轮询
      setTimeout(() => {
        if (this.inferencing) {
          clearInterval(pollInterval);
          this.inferenceError = '推理超时，请检查模型路径是否正确或尝试简化输入内容';
          this.inferencing = false;
        }
      }, 300000); // 5分钟超时
    }
  }
}
</script>

<style scoped>
.response-text {
  white-space: pre-wrap;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  font-size: 1rem;
  line-height: 1.5;
}

.inference-result {
  max-height: 500px;
  overflow-y: auto;
}
</style> 