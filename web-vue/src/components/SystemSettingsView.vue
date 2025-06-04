<template>
  <div class="system-settings-view">
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-primary text-white d-flex justify-content-between align-items-center">
            <div>
              <i class="bi bi-gear me-2"></i>系统设置
            </div>
            <button class="btn btn-light btn-sm" @click="refreshSystemInfo">
              <i class="bi bi-arrow-clockwise me-1"></i>刷新
            </button>
          </div>
          <div class="card-body">
            <div v-if="loading" class="d-flex justify-content-center my-5">
              <div class="spinner-border text-primary me-2" role="status">
                <span class="visually-hidden">加载中...</span>
              </div>
              <span>正在获取系统信息...</span>
            </div>
            
            <div v-else-if="error" class="alert alert-danger">
              <i class="bi bi-exclamation-triangle me-2"></i>
              加载系统信息失败：{{ error }}
              <button class="btn btn-sm btn-danger ms-3" @click="refreshSystemInfo">重试</button>
            </div>
            
            <div v-else>
              <!-- 系统信息 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-info-circle me-2"></i>系统信息
              </h5>
              <div class="row mb-4">
                <div class="col-md-6">
                  <table class="table">
                    <tbody>
                      <tr>
                        <th scope="row">操作系统</th>
                        <td>{{ systemInfo.os || '未知' }}</td>
                      </tr>
                      <tr>
                        <th scope="row">Python版本</th>
                        <td>{{ systemInfo.python_version || '未知' }}</td>
                      </tr>
                      <tr>
                        <th scope="row">Transformers版本</th>
                        <td>{{ systemInfo.transformers_version || '未安装' }}</td>
                      </tr>
                      <tr>
                        <th scope="row">PyTorch版本</th>
                        <td>{{ systemInfo.pytorch_version || '未安装' }}</td>
                      </tr>
                    </tbody>
                  </table>
                </div>
                <div class="col-md-6">
                  <table class="table">
                    <tbody>
                      <tr>
                        <th scope="row">CUDA版本</th>
                        <td>{{ systemInfo.cuda_version || '未检测到' }}</td>
                      </tr>
                      <tr>
                        <th scope="row">cuDNN版本</th>
                        <td>{{ systemInfo.cudnn_version || '未知' }}</td>
                      </tr>
                      <tr>
                        <th scope="row">系统内存</th>
                        <td>{{ formatMemory(systemInfo.memory_available) }} 可用 / {{ formatMemory(systemInfo.memory_total) }} 总计</td>
                      </tr>
                      <tr>
                        <th scope="row">磁盘空间</th>
                        <td>{{ formatDiskSpace(systemInfo.disk_space) }}</td>
                      </tr>
                    </tbody>
                  </table>
                </div>
              </div>
              
              <!-- 未来会实现的设置 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-sliders me-2"></i>通用设置
              </h5>
              <form>
                <div class="row mb-3">
                  <div class="col-md-6">
                    <div class="form-check form-switch">
                      <input class="form-check-input" type="checkbox" id="darkMode" disabled>
                      <label class="form-check-label" for="darkMode">深色模式</label>
                    </div>
                  </div>
                  <div class="col-md-6">
                    <div class="form-check form-switch">
                      <input class="form-check-input" type="checkbox" id="autoUpdate" disabled>
                      <label class="form-check-label" for="autoUpdate">自动检查更新</label>
                    </div>
                  </div>
                </div>
                
                <div class="row mb-3">
                  <div class="col-md-6">
                    <label for="cacheLocation" class="form-label">缓存位置</label>
                    <div class="input-group">
                      <input type="text" class="form-control" id="cacheLocation" value="./cache" disabled>
                      <button class="btn btn-outline-secondary" type="button" disabled>浏览...</button>
                    </div>
                  </div>
                  <div class="col-md-6">
                    <label for="maxCacheSize" class="form-label">最大缓存大小 (GB)</label>
                    <input type="number" class="form-control" id="maxCacheSize" value="10" disabled>
                  </div>
                </div>
                
                <h5 class="card-title border-bottom pb-2 mt-4">
                  <i class="bi bi-hdd-stack me-2"></i>默认模型设置
                </h5>
                <div class="row mb-3">
                  <div class="col-md-6">
                    <label for="defaultModelPath" class="form-label">默认模型路径</label>
                    <div class="input-group">
                      <input type="text" class="form-control" id="defaultModelPath" value="./ckpt/qwen" disabled>
                      <button class="btn btn-outline-secondary" type="button" disabled>浏览...</button>
                    </div>
                  </div>
                  <div class="col-md-6">
                    <label for="precision" class="form-label">默认精度</label>
                    <select class="form-select" id="precision" disabled>
                      <option>BF16 (自动)</option>
                      <option>FP16</option>
                      <option>FP32</option>
                    </select>
                  </div>
                </div>
                
                <div class="d-grid gap-2 col-6 mx-auto mt-4">
                  <button type="submit" class="btn btn-primary" disabled>
                    <i class="bi bi-save me-2"></i>保存设置
                  </button>
                </div>
              </form>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'SystemSettingsView',
  data() {
    return {
      systemInfo: {
        os: '',
        python_version: '',
        transformers_version: '',
        pytorch_version: '',
        cuda_version: '',
        cudnn_version: '',
        memory_total: 0,
        memory_available: 0,
        disk_space: 0
      },
      loading: true,
      error: null
    }
  },
  mounted() {
    this.refreshSystemInfo();
  },
  methods: {
    refreshSystemInfo() {
      this.loading = true;
      this.error = null;
      
      fetch('/api/system/info')
        .then(response => {
          if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
          }
          return response.json();
        })
        .then(data => {
          if (data.success && data.data) {
            this.systemInfo = data.data;
          } else {
            throw new Error(data.message || '获取系统信息失败');
          }
        })
        .catch(error => {
          console.error('获取系统信息出错:', error);
          this.error = error.message || '网络错误，请稍后再试';
        })
        .finally(() => {
          this.loading = false;
        });
    },
    formatMemory(mb) {
      if (!mb) return '未知';
      return mb > 1024 ? `${(mb / 1024).toFixed(1)} GB` : `${mb} MB`;
    },
    formatDiskSpace(mb) {
      if (!mb) return '未知';
      return mb > 1024 ? `${(mb / 1024).toFixed(1)} GB` : `${mb} MB`;
    }
  }
}
</script>

<style scoped>
.table th {
  width: 40%;
  font-weight: 600;
}
</style> 