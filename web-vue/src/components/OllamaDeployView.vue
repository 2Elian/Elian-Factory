<template>
  <div class="ollama-deploy-view">
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-primary text-white">
            <i class="bi bi-box-arrow-up me-2"></i>部署到Ollama
          </div>
          <div class="card-body">
            <form @submit.prevent="deployToOllama">
              <!-- 模型路径 -->
              <div class="mb-3">
                <label for="model_path" class="form-label">模型路径</label>
                <input 
                  type="text" 
                  class="form-control" 
                  id="model_path" 
                  v-model="deployData.model_path"
                  placeholder="输入模型路径，例如：/output/your_task/lora/weight"
                  required
                >
                <div class="form-text">微调后的模型路径，通常是训练输出目录下的weight文件夹</div>
              </div>
              
              <!-- 模型名称 -->
              <div class="mb-3">
                <label for="model_name" class="form-label">Ollama模型名称</label>
                <input 
                  type="text" 
                  class="form-control" 
                  id="model_name" 
                  v-model="deployData.model_name"
                  placeholder="输入Ollama模型名称，例如：my-model"
                  required
                >
                <div class="form-text">在Ollama中使用的模型名称，只能包含字母、数字、连字符和下划线</div>
              </div>
              
              <!-- 提交按钮 -->
              <div class="d-grid gap-2 col-6 mx-auto">
                <button 
                  type="submit" 
                  class="btn btn-primary"
                  :disabled="deploying"
                >
                  <i class="bi bi-box-arrow-up me-2"></i>
                  {{ deploying ? '部署中...' : '开始部署' }}
                </button>
              </div>
            </form>
          </div>
        </div>
      </div>
    </div>
    
    <!-- 部署结果 -->
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-success text-white">
            <i class="bi bi-check-circle me-2"></i>部署结果
          </div>
          <div class="card-body">
            <div v-if="deploying" class="loading-spinner">
              <div class="spinner-border text-primary" role="status">
                <span class="visually-hidden">Loading...</span>
              </div>
              <p class="ms-2">模型部署中，请稍候...</p>
            </div>
            <div v-else-if="deployError" class="alert alert-danger">
              <i class="bi bi-exclamation-circle me-2"></i>{{ deployError }}
            </div>
            <div v-else-if="deployResult" class="alert alert-success">
              <i class="bi bi-check-circle me-2"></i>{{ deployResult }}
            </div>
            <div v-else class="text-center text-muted">
              <i class="bi bi-info-circle me-2"></i>在上方输入信息后点击"开始部署"按钮
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'OllamaDeployView',
  data() {
    return {
      deployData: {
        model_path: '',
        model_name: ''
      },
      deploying: false,
      deployResult: null,
      deployError: null
    }
  },
  methods: {
    deployToOllama() {
      this.deploying = true
      this.deployError = null
      this.deployResult = null
      
      // 验证模型名称格式
      const modelNameRegex = /^[a-zA-Z0-9_-]+$/
      if (!modelNameRegex.test(this.deployData.model_name)) {
        this.deployError = '模型名称只能包含字母、数字、连字符和下划线'
        this.deploying = false
        return
      }
      
      // 调用后端API
      fetch('/api/ollama/deploy', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(this.deployData)
      })
      .then(response => {
        if (!response.ok) {
          throw new Error('部署请求失败');
        }
        return response.json();
      })
      .then(data => {
        if (!data.success) {
          throw new Error(data.message || '部署请求失败');
        }
        
        // 开始轮询部署状态
        this.pollDeployStatus(data.task_id);
      })
      .catch(error => {
        this.deployError = `部署失败: ${error.message}`;
        this.deploying = false;
      });
    },
    
    // 轮询部署状态
    pollDeployStatus(taskId) {
      const pollInterval = setInterval(() => {
        fetch(`/api/ollama/status?task_id=${taskId}`)
          .then(response => {
            if (!response.ok) {
              throw new Error('获取部署状态失败');
            }
            return response.json();
          })
          .then(data => {
            if (data.success) {
              if (data.status === 'completed') {
                // 清除轮询
                clearInterval(pollInterval);
                
                // 显示结果
                this.deployResult = `模型部署成功！使用 "ollama run ${this.deployData.model_name}" 即可使用您微调好的模型。`;
                this.deploying = false;
              } else if (data.status === 'failed') {
                // 清除轮询
                clearInterval(pollInterval);
                
                this.deployError = `部署失败: ${data.message || '未知错误'}`;
                this.deploying = false;
              }
            }
          })
          .catch(error => {
            // 清除轮询
            clearInterval(pollInterval);
            
            this.deployError = `获取部署状态失败: ${error.message}`;
            this.deploying = false;
          });
      }, 3000); // 每3秒轮询一次
      
      // 设置超时，避免无限轮询
      setTimeout(() => {
        if (this.deploying) {
          clearInterval(pollInterval);
          this.deployError = '部署超时，请检查模型路径是否正确或Ollama是否已安装';
          this.deploying = false;
        }
      }, 300000); // 5分钟超时
    }
  }
}
</script>

<style scoped>
/* 特定组件样式可以在这里添加 */
</style> 