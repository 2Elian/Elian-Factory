<template>
  <div>
    <!-- GPU资源监控 -->
    <div class="row mt-3" id="gpu-info-container">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-primary text-white">
            <i class="bi bi-gpu-card me-2"></i>GPU资源监控
            <button class="btn btn-sm btn-light float-end" @click="fetchGPUInfo">
              <i class="bi bi-arrow-clockwise me-1"></i> 刷新
            </button>
          </div>
          <div class="card-body">
            <div v-if="loading" class="loading-spinner">
              <div class="spinner-border text-primary" role="status">
                <span class="visually-hidden">Loading...</span>
              </div>
              <p class="ms-2">正在检测GPU...</p>
            </div>
            <div v-else-if="gpuError" class="alert alert-danger">
              <i class="bi bi-exclamation-circle me-2"></i>{{ gpuError }}
            </div>
            <div v-else-if="!gpus.length" class="alert alert-warning">
              <i class="bi bi-exclamation-triangle me-2"></i>未检测到可用的GPU
            </div>
            <div v-else>
              <GpuCard 
                v-for="gpu in gpus" 
                :key="gpu.id" 
                :gpu="gpu" 
              />
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 训练参数表单 -->
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-success text-white">
            <i class="bi bi-sliders me-2"></i>训练参数配置
          </div>
          <div class="card-body">
            <form @submit.prevent="startTraining">
              <!-- 模型配置 -->
              <h5 class="card-title border-bottom pb-2">
                <i class="bi bi-hdd-stack me-2"></i>模型配置
              </h5>
              <div class="row mb-3">
                <div class="col-md-6">
                  <label for="model_name_or_path" class="form-label">模型路径</label>
                  <input 
                    type="text" 
                    class="form-control" 
                    id="model_name_or_path" 
                    v-model="formData.model_name_or_path"
                    required
                  >
                  <div class="form-text">本地模型文件夹路径</div>
                </div>
                <div class="col-md-6">
                  <label for="output_dir" class="form-label">输出目录</label>
                  <input 
                    type="text" 
                    class="form-control" 
                    id="output_dir" 
                    v-model="formData.output_dir"
                    required
                  >
                  <div class="form-text">训练结果保存位置</div>
                </div>
              </div>

              <!-- 数据配置 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-file-earmark-text me-2"></i>数据配置
              </h5>
              <div class="row mb-3">
                <div class="col-md-6">
                  <label for="train_file" class="form-label">训练数据文件</label>
                  <div class="input-group mb-1">
                    <select 
                      class="form-select" 
                      id="train_file" 
                      v-model="formData.train_file"
                      required
                      @change="previewDataFile"
                    >
                      <option value="" disabled>请选择训练数据文件</option>
                      <option v-for="file in dataFiles" :key="file" :value="'data/' + file">{{ file }}</option>
                    </select>
                    <button class="btn btn-outline-secondary" type="button" @click="refreshDataFiles">
                      <i class="bi bi-arrow-clockwise"></i>
                    </button>
                  </div>
                  <div class="form-text">选择JSONL格式的训练数据</div>
                </div>
                <div class="col-md-6">
                  <label for="max_seq_length" class="form-label">最大序列长度</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="max_seq_length" 
                    v-model.number="formData.max_seq_length"
                    min="1"
                    required
                  >
                  <div class="form-text">输入序列的最大长度</div>
                </div>
              </div>

              <!-- 数据预览面板 -->
              <div class="row mb-3" v-if="dataPreview">
                <div class="col-12">
                  <div class="card">
                    <div class="card-header bg-light">
                      <i class="bi bi-file-text me-2"></i>数据预览
                    </div>
                    <div class="card-body">
                      <pre class="data-preview">{{ dataPreview }}</pre>
                    </div>
                  </div>
                </div>
              </div>

              <!-- 训练超参数 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-gear-wide-connected me-2"></i>训练超参数
              </h5>
              <div class="row mb-3">
                <div class="col-md-4">
                  <label for="num_train_epochs" class="form-label">训练轮数</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="num_train_epochs" 
                    v-model.number="formData.num_train_epochs"
                    min="1"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="per_device_train_batch_size" class="form-label">批次大小</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="per_device_train_batch_size" 
                    v-model.number="formData.per_device_train_batch_size"
                    min="1"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="gradient_accumulation_steps" class="form-label">梯度累积步数</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="gradient_accumulation_steps" 
                    v-model.number="formData.gradient_accumulation_steps"
                    min="1"
                    required
                  >
                </div>
              </div>
              <div class="row mb-3">
                <div class="col-md-4">
                  <label for="learning_rate" class="form-label">学习率</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="learning_rate" 
                    v-model.number="formData.learning_rate"
                    step="0.0001"
                    min="0"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="lr_scheduler_type" class="form-label">学习率调度器</label>
                  <select 
                    class="form-select" 
                    id="lr_scheduler_type" 
                    v-model="formData.lr_scheduler_type"
                    required
                  >
                    <option value="constant_with_warmup">常数预热</option>
                    <option value="linear">线性衰减</option>
                    <option value="cosine">余弦衰减</option>
                    <option value="polynomial">多项式衰减</option>
                  </select>
                </div>
                <div class="col-md-4">
                  <label for="warmup_steps" class="form-label">预热步数</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="warmup_steps" 
                    v-model.number="formData.warmup_steps"
                    min="0"
                    required
                  >
                </div>
              </div>

              <!-- LoRA参数 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-layers me-2"></i>LoRA参数
              </h5>
              <div class="row mb-3">
                <div class="col-md-4">
                  <label for="lora_rank" class="form-label">LoRA秩 (r)</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="lora_rank" 
                    v-model.number="formData.lora_rank"
                    min="1"
                    required
                  >
                  <div class="form-text">越大效果越好，但训练成本更高</div>
                </div>
                <div class="col-md-4">
                  <label for="lora_alpha" class="form-label">LoRA Alpha</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="lora_alpha" 
                    v-model.number="formData.lora_alpha"
                    min="1"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="lora_dropout" class="form-label">LoRA Dropout</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="lora_dropout" 
                    v-model.number="formData.lora_dropout"
                    step="0.01"
                    min="0"
                    max="1"
                    required
                  >
                </div>
              </div>

              <!-- 保存和日志设置 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-save me-2"></i>保存和日志设置
              </h5>
              <div class="row mb-3">
                <div class="col-md-4">
                  <label for="logging_steps" class="form-label">日志记录步数</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="logging_steps" 
                    v-model.number="formData.logging_steps"
                    min="1"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="save_steps" class="form-label">保存检查点步数</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="save_steps" 
                    v-model.number="formData.save_steps"
                    min="1"
                    required
                  >
                </div>
                <div class="col-md-4">
                  <label for="save_total_limit" class="form-label">最大保存检查点数</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="save_total_limit" 
                    v-model.number="formData.save_total_limit"
                    min="1"
                    required
                  >
                </div>
              </div>

              <!-- 优化设置 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-lightning-charge me-2"></i>优化设置
              </h5>
              <div class="row mb-3">
                <div class="col-md-4">
                  <label for="optim" class="form-label">优化器</label>
                  <select 
                    class="form-select" 
                    id="optim" 
                    v-model="formData.optim"
                    required
                  >
                    <option value="adamw_torch">AdamW (PyTorch)</option>
                    <option value="adamw_hf">AdamW (Hugging Face)</option>
                    <option value="adafactor">Adafactor</option>
                  </select>
                </div>
                <div class="col-md-4">
                  <label for="train_mode" class="form-label">训练模式</label>
                  <select 
                    class="form-select" 
                    id="train_mode" 
                    v-model="formData.train_mode"
                    required
                  >
                    <option value="lora">LoRA</option>
                    <option value="qlora">QLoRA</option>
                  </select>
                </div>
                <div class="col-md-4">
                  <label for="seed" class="form-label">随机种子</label>
                  <input 
                    type="number" 
                    class="form-control" 
                    id="seed" 
                    v-model.number="formData.seed"
                    min="0"
                    required
                  >
                </div>
              </div>

              <!-- 高级选项 -->
              <h5 class="card-title border-bottom pb-2 mt-4">
                <i class="bi bi-puzzle me-2"></i>高级选项
              </h5>
              <div class="row mb-3">
                <div class="col-md-4">
                  <div class="form-check form-switch">
                    <input 
                      class="form-check-input" 
                      type="checkbox" 
                      id="fp16" 
                      v-model="formData.fp16"
                    >
                    <label class="form-check-label" for="fp16">启用FP16训练</label>
                  </div>
                </div>
                <div class="col-md-4">
                  <div class="form-check form-switch">
                    <input 
                      class="form-check-input" 
                      type="checkbox" 
                      id="gradient_checkpointing" 
                      v-model="formData.gradient_checkpointing"
                    >
                    <label class="form-check-label" for="gradient_checkpointing">梯度检查点</label>
                  </div>
                </div>
                <div class="col-md-4">
                  <div class="form-check form-switch">
                    <input 
                      class="form-check-input" 
                      type="checkbox" 
                      id="distributed" 
                      v-model="formData.distributed"
                    >
                    <label class="form-check-label" for="distributed">分布式训练</label>
                  </div>
                </div>
              </div>

              <!-- 提交按钮 -->
              <div class="d-grid gap-2 col-6 mx-auto mt-4">
                <div class="d-flex justify-content-center">
                  <button 
                    type="submit" 
                    class="btn btn-primary btn-lg me-3" 
                    :disabled="formSubmitting"
                  >
                    <i class="bi bi-play-circle me-2"></i>
                    {{ formSubmitting ? '训练中...' : '开始训练' }}
                  </button>
                  <button 
                    type="button" 
                    class="btn btn-success btn-lg" 
                    @click="saveConfig"
                  >
                    <i class="bi bi-save me-2"></i>
                    保存配置
                  </button>
                </div>
              </div>
            </form>
          </div>
        </div>
      </div>
    </div>

    <!-- 训练状态 -->
    <div class="row mt-3 mb-4">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-info text-white">
            <i class="bi bi-activity me-2"></i>训练状态
          </div>
          <div class="card-body">
            <div v-if="trainingStatus" class="training-status">
              <div :class="['alert', statusClass]">
                <i :class="['bi', statusIcon, 'me-2']"></i>
                <span>{{ trainingStatus }}</span>
              </div>
              <div class="progress" v-if="trainingStarted && !formSubmitting">
                <div 
                  class="progress-bar progress-bar-striped progress-bar-animated" 
                  role="progressbar" 
                  style="width: 100%"
                ></div>
              </div>
              <!-- 失败状态显示 -->
              <div v-if="statusClass === 'alert-danger'" class="mt-2">
                <a href="#" class="text-danger" @click.prevent="resetForm">
                  <i class="bi bi-arrow-counterclockwise me-1"></i> 重置参数
                </a>
              </div>
            </div>
            <div v-else class="text-center text-muted">
              <i class="bi bi-hourglass me-2"></i>没有正在进行的训练任务
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 训练日志 -->
    <div class="row mt-3 mb-4">
      <div class="col-12">
        <div class="card logs-card">
          <div class="card-header bg-dark text-white d-flex justify-content-between align-items-center">
            <div>
              <i class="bi bi-terminal me-2"></i>训练日志
            </div>
            <div class="d-flex gap-2">
              <div class="form-check form-switch me-2">
                <input class="form-check-input" type="checkbox" id="autoScrollSwitch" v-model="autoScroll">
                <label class="form-check-label small text-light" for="autoScrollSwitch">自动滚动</label>
              </div>
              <button class="btn btn-sm btn-outline-light" @click="refreshLogs" title="刷新日志">
                <i class="bi bi-arrow-clockwise"></i>
              </button>
            </div>
          </div>
          <div class="card-body p-0">
            <div v-if="logsLoading" class="d-flex justify-content-center align-items-center py-5">
              <div class="spinner-border text-primary me-2" role="status">
                <span class="visually-hidden">加载中...</span>
              </div>
              <span>正在加载日志...</span>
            </div>
            <div v-else-if="!formattedLogs" class="text-center py-5 text-muted">
              <i class="bi bi-file-earmark-text fs-3 mb-2 d-block"></i>
              <span>暂无训练日志或训练尚未开始...</span>
            </div>
            <div v-else class="training-logs-container">
              <pre class="training-logs" @scroll="handleLogScroll" v-html="formattedLogs"></pre>
              <button 
                v-if="!autoScroll" 
                class="scroll-to-bottom-btn" 
                @click="scrollToBottom" 
                title="跳转到最新日志">
                <i class="bi bi-arrow-down-circle-fill"></i>
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import GpuCard from './GpuCard.vue'

export default {
  name: 'TrainingView',
  components: {
    GpuCard
  },
  data() {
    return {
      gpus: [],
      loading: true,
      gpuError: null,
      formData: {
        model_name_or_path: '/ckpt/ds',
        output_dir: '/output/your_task/lora',
        train_file: '',
        num_train_epochs: 100,
        per_device_train_batch_size: 2,
        gradient_accumulation_steps: 4,
        learning_rate: 0.0002,
        max_seq_length: 100,
        logging_steps: 1,
        save_steps: 2,
        save_total_limit: 1,
        lr_scheduler_type: 'constant_with_warmup',
        warmup_steps: 1,
        lora_rank: 8,
        lora_alpha: 16,
        lora_dropout: 0.05,
        gradient_checkpointing: true,
        optim: 'adamw_torch',
        train_mode: 'lora',
        seed: 42,
        fp16: false,
        distributed: false
      },
      defaultFormData: null,
      formSubmitting: false,
      trainingStatus: null,
      trainingStarted: false,
      statusClass: 'alert-info',
      statusIcon: 'bi-info-circle',
      dataFiles: [],
      dataPreview: null,
      trainingLogs: '',
      logsPolling: null,
      logsLoading: false,
      autoScroll: true,
      previousLogLength: 0,
      configName: '',
      savedConfigs: [],
      configLoading: false,
      configError: null
    }
  },
  computed: {
    formattedLogs() {
      if (!this.trainingLogs) return '';
      
      // 高亮关键词
      let formatted = this.trainingLogs
        // 替换错误信息
        .replace(/(error|失败|错误|Exception|exception)/gi, '<span style="color:#f38ba8;font-weight:bold;">$1</span>')
        // 替换成功信息
        .replace(/(success|成功|完成|完毕)/gi, '<span style="color:#a6e3a1;font-weight:bold;">$1</span>')
        // 替换警告信息
        .replace(/(warning|警告)/gi, '<span style="color:#f9e2af;font-weight:bold;">$1</span>')
        // 替换训练相关信息
        .replace(/(train|训练|epoch|batch|step|loss)/gi, '<span style="color:#89b4fa;font-weight:bold;">$1</span>')
        // 替换百分比进度
        .replace(/(\d+%)/g, '<span style="color:#f5c2e7;font-weight:bold;">$1</span>')
        // 替换Elian标识
        .replace(/(@Elian)/g, '<span style="color:#fab387;font-weight:bold;">$1</span>')
        // 替换星号装饰
        .replace(/(🌟+|☀️+)/g, '<span style="color:#fab387;">$1</span>')
        // 高亮进度条
        .replace(/(\|█+[ █]*\|)/g, '<span style="color:#89dceb;">$1</span>')
        // 高亮时间戳
        .replace(/(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})/g, '<span style="color:#94e2d5;">$1</span>')
        // 高亮括号内容
        .replace(/(\([^)]+\))/g, '<span style="color:#cba6f7;">$1</span>');
      
      return formatted;
    }
  },
  mounted() {
    this.fetchGPUInfo()
    this.fetchDefaultConfig()
    this.fetchDataFiles()
    
    // 首次加载日志时显示加载动画
    this.fetchTrainingLogs(true)
  },
  methods: {
    fetchGPUInfo() {
      this.loading = true
      this.gpuError = null
      
      fetch('/api/gpu/status')
        .then(response => {
          if (!response.ok) {
            throw new Error('GPU检测失败')
          }
          return response.json()
        })
        .then(data => {
          if (data.success && data.data) {
            this.gpus = data.data
          } else {
            throw new Error(data.message || 'GPU检测失败')
          }
          this.loading = false
        })
        .catch(error => {
          console.error('获取GPU信息失败:', error)
          this.gpuError = `获取GPU信息失败: ${error.message}`
          this.loading = false
        })
    },
    fetchDefaultConfig() {
      fetch('/api/default-config')
        .then(response => {
          if (!response.ok) {
            throw new Error('获取默认配置失败')
          }
          return response.json()
        })
        .then(data => {
          if (data.success) {
            const { success, ...configData } = data
            this.formData = { ...configData }
            this.defaultFormData = { ...configData } // 保存默认值的副本
          } else {
            throw new Error(data.message || '获取默认配置失败')
          }
        })
        .catch(error => {
          console.error('获取默认配置失败:', error)
        })
    },
    resetForm() {
      if (this.defaultFormData) {
        this.formData = { ...this.defaultFormData }
      }
    },
    startTraining() {
      this.formSubmitting = true
      this.trainingStatus = '训练任务正在启动...'
      this.statusClass = 'alert-info'
      this.statusIcon = 'bi-info-circle'
      // 重置训练状态，移除进度条
      this.trainingStarted = false
      
      // 直接发送训练请求，不再先保存配置
      fetch('/api/train', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(this.formData)
      })
      .then(response => {
        if (!response.ok) {
          this.formSubmitting = false;
          this.trainingStarted = false;
          throw new Error('启动训练任务失败，服务器返回：' + response.status);
        }
        return response.json();
      })
      .then(data => {
        // 检查后端返回的结果
        if (!data.success) {
          // 如果后端明确返回失败
          this.trainingStatus = data.message || '训练启动失败';
          this.statusClass = 'alert-danger';
          this.statusIcon = 'bi-exclamation-circle';
          this.formSubmitting = false;
          this.trainingStarted = false; // 确保不显示进度条
          
          // 如果有详细错误信息，则显示在日志区域
          if (data.error) {
            this.trainingLogs = `错误详情: ${data.error}\n\n`;
          }
          return;
        }
        
        this.trainingStatus = data.message || '训练任务已成功启动';
        this.statusClass = 'alert-success';
        this.statusIcon = 'bi-check-circle-fill';
        this.trainingStarted = true; // 只有在成功时才显示进度条
        
        // 开始获取日志
        this.startLogsPolling();
      })
      .catch(error => {
        console.error('训练请求失败:', error);
        this.trainingStatus = `训练请求失败: ${error.message}`;
        this.statusClass = 'alert-danger';
        this.statusIcon = 'bi-exclamation-circle';
        this.formSubmitting = false;
        this.trainingStarted = false; // 确保不显示进度条
      });
    },
    fetchDataFiles() {
      fetch('/api/data/files')
        .then(response => {
          if (!response.ok) {
            throw new Error('获取数据文件列表失败')
          }
          return response.json()
        })
        .then(data => {
          if (data.success) {
            this.dataFiles = data.files
          } else {
            throw new Error(data.message || '获取数据文件列表失败')
          }
        })
        .catch(error => {
          console.error('获取数据文件列表失败:', error)
          alert('获取数据文件列表失败: ' + error.message)
        })
    },
    refreshDataFiles() {
      this.dataPreview = null
      this.fetchDataFiles()
    },
    previewDataFile() {
      // 只获取文件名部分
      if (!this.formData.train_file) {
        this.dataPreview = null
        return
      }
      
      const filename = this.formData.train_file.split('/').pop()
      
      fetch(`/api/data/preview?file=${encodeURIComponent(filename)}`)
        .then(response => {
          if (!response.ok) {
            throw new Error('获取数据预览失败')
          }
          return response.json()
        })
        .then(data => {
          if (data.success) {
            this.dataPreview = data.content
          } else {
            throw new Error(data.message || '获取数据预览失败')
          }
        })
        .catch(error => {
          console.error('获取数据预览失败:', error)
          this.dataPreview = `获取预览失败: ${error.message}`
        })
    },
    // 开始轮询获取训练日志
    startLogsPolling() {
      // 清除现有的轮询器
      this.stopLogsPolling()
      
      // 创建新的轮询器，每2秒获取一次日志
      this.logsPolling = setInterval(() => {
        this.fetchTrainingLogs(false) // 自动轮询不显示加载动画
      }, 2000)
    },
    
    // 停止轮询
    stopLogsPolling() {
      if (this.logsPolling) {
        clearInterval(this.logsPolling)
        this.logsPolling = null
      }
    },
    
    // 获取训练日志
    fetchTrainingLogs(showLoading = true) {
      // 只在手动刷新时显示加载动画
      if (showLoading) {
        this.logsLoading = true;
      }
      
      // 检查是否需要保留自动滚动状态
      const logsElement = document.querySelector('.training-logs');
      let isScrolledToBottom = false;
      
      if (logsElement) {
        // 判断用户是否已经滚动到接近底部
        isScrolledToBottom = Math.abs(
          (logsElement.scrollHeight - logsElement.scrollTop - logsElement.clientHeight) < 50
        );
        // 如果用户手动滚动了，则更新自动滚动标志
        if (this.previousLogLength > 0 && !isScrolledToBottom) {
          this.autoScroll = false;
        }
      }
      
      fetch('/api/train/logs')
        .then(response => {
          if (!response.ok) {
            throw new Error('获取训练日志失败')
          }
          return response.json()
        })
        .then(data => {
          if (showLoading) {
            this.logsLoading = false;
          }
          
          if (data.success) {
            const newLogs = data.logs || '';
            // 记录当前日志长度，用于下次比较
            this.previousLogLength = newLogs.length;
            this.trainingLogs = newLogs;
            
            // 使用nextTick确保DOM已更新
            this.$nextTick(() => {
              // 如果设置了自动滚动，或者用户已经滚动到底部，则滚动到底部
              if (this.autoScroll) {
                const updatedLogsElement = document.querySelector('.training-logs');
                if (updatedLogsElement) {
                  updatedLogsElement.scrollTop = updatedLogsElement.scrollHeight;
                }
              }
            });
          }
        })
        .catch(error => {
          if (showLoading) {
            this.logsLoading = false;
          }
          console.error('获取训练日志失败:', error);
        });
    },

    // 添加日志滚动处理函数
    handleLogScroll(event) {
      const logsElement = event.target;
      // 检查用户是否手动滚动到了底部
      const isScrolledToBottom = Math.abs(
        (logsElement.scrollHeight - logsElement.scrollTop - logsElement.clientHeight) < 50
      );
      // 更新自动滚动标志
      this.autoScroll = isScrolledToBottom;
    },

    // 直接更新配置到llm/configs/default_config.json中
    saveConfig() {
      console.log('更新配置到llm/configs/default_config.json中')
      
      // 构造要保存的配置数据
      const configData = { ...this.formData }
      console.log('配置参数信息为：',configData)
      
      return fetch('/api/config/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          config_name: "default_config",
          config_data: configData
        })
      })
      .then(response => {
        console.log('配置保存响应状态:', response.status, response.statusText)
        if (!response.ok) {
          throw new Error(`保存配置失败，HTTP状态: ${response.status} ${response.statusText}`)
        }
        return response.json()
      })
      .then(data => {
        console.log('配置保存响应数据:', data)
        if (data.success) {
          console.log('配置保存成功，路径:', data.path)
          alert('配置已保存成功！')
          return data
        } else {
          throw new Error(data.error || data.message || '保存配置失败')
        }
      })
      .catch(error => {
        console.error('保存配置失败:', error)
        alert('保存配置失败: ' + error.message)
        throw error // 传递错误以便链式调用可以捕获
      })
    },
    
    refreshConfigs() {
      this.configLoading = true
      this.configError = null
      this.savedConfigs = []
      
      fetch('/api/config/list')
        .then(response => {
          if (!response.ok) {
            throw new Error('获取配置列表失败')
          }
          return response.json()
        })
        .then(data => {
          this.configLoading = false
          if (data.success) {
            this.savedConfigs = data.configs.map(name => ({ name }))
          } else {
            throw new Error(data.message || '获取配置列表失败')
          }
        })
        .catch(error => {
          console.error('获取配置列表失败:', error)
          this.configError = '获取配置列表失败: ' + error.message
          this.configLoading = false
        })
    },
    
    loadConfig(configName) {
      fetch(`/api/config/load?name=${encodeURIComponent(configName)}`)
        .then(response => {
          if (!response.ok) {
            throw new Error('加载配置失败')
          }
          return response.json()
        })
        .then(data => {
          if (data.success && data.config) {
            // 更新表单数据
            this.formData = { ...data.config }
            if (this.loadConfigModal) {
              this.loadConfigModal.hide()
            }
            
            // 更新数据预览（如果有加载数据文件）
            if (this.formData.train_file) {
              this.previewDataFile()
            }
          } else {
            throw new Error(data.message || '加载配置失败')
          }
        })
        .catch(error => {
          console.error('加载配置失败:', error)
          alert('加载配置失败: ' + error.message)
        })
    },
    
    deleteConfig(configName) {
      if (!confirm(`确定要删除配置 "${configName}" 吗？`)) {
        return
      }
      
      fetch(`/api/config/delete?name=${encodeURIComponent(configName)}`, { method: 'DELETE' })
        .then(response => {
          if (!response.ok) {
            throw new Error('删除配置失败')
          }
          return response.json()
        })
        .then(data => {
          if (data.success) {
            // 刷新配置列表
            this.refreshConfigs()
          } else {
            throw new Error(data.message || '删除配置失败')
          }
        })
        .catch(error => {
          console.error('删除配置失败:', error)
          alert('删除配置失败: ' + error.message)
        })
    },
    
    // 刷新日志 (手动刷新显示加载动画)
    refreshLogs() {
      this.fetchTrainingLogs(true);
    },

    // 添加跳转到底部方法
    scrollToBottom() {
      const logsElement = document.querySelector('.training-logs');
      if (logsElement) {
        logsElement.scrollTop = logsElement.scrollHeight;
        this.autoScroll = true;
      }
    }
  },
  beforeUnmount() {
    this.stopLogsPolling()
  }
}
</script>

<style scoped>
/* 组件特定样式可以在这里添加 */
.data-preview {
  max-height: 200px;
  overflow-y: auto;
  background-color: #f8f9fa;
  padding: 10px;
  font-size: 0.9rem;
  white-space: pre-wrap;
  word-break: break-all;
}

.logs-card {
  border: none;
  border-radius: 10px;
  box-shadow: 0 5px 15px rgba(0,0,0,0.1);
  overflow: hidden;
}

.logs-card .card-header {
  border-bottom: none;
  padding: 15px;
}

.training-logs-container {
  position: relative;
}

.training-logs {
  max-height: 500px;
  overflow-y: auto;
  background-color: #1e1e2e;
  color: #cdd6f4;
  padding: 15px;
  font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
  font-size: 0.9rem;
  line-height: 1.5;
  white-space: pre-wrap;
  word-break: break-word;
  border-radius: 0;
  box-shadow: none;
  border-left: none;
  margin: 0;
  position: relative; /* 添加相对定位，作为跳转按钮的参考 */
}

/* 自定义滚动条样式 */
.training-logs::-webkit-scrollbar {
  width: 8px;
}

.training-logs::-webkit-scrollbar-track {
  background: #313244;
  border-radius: 4px;
}

.training-logs::-webkit-scrollbar-thumb {
  background: #89b4fa;
  border-radius: 4px;
}

.training-logs::-webkit-scrollbar-thumb:hover {
  background: #a6adc8;
}

/* 添加一些关键词高亮 */
.training-logs .highlight {
  color: #f38ba8;
  font-weight: bold;
}

.config-preview {
  max-height: 250px;
  overflow-y: auto;
  background-color: #f8f9fa;
  padding: 10px;
  font-size: 0.8rem;
  white-space: pre-wrap;
  word-break: break-all;
  border-radius: 4px;
}

/* 添加跳转到底部按钮样式 */
.scroll-to-bottom-btn {
  position: absolute;
  right: 20px;
  bottom: 20px;
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background-color: #89b4fa;
  color: #1e1e2e;
  border: none;
  box-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s ease;
  z-index: 10;
  opacity: 0.8;
}

.scroll-to-bottom-btn:hover {
  background-color: #b4befe;
  opacity: 1;
  transform: translateY(-2px);
}

.scroll-to-bottom-btn i {
  font-size: 1.2rem;
}
</style> 