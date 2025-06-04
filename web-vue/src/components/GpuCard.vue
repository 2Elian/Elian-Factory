<template>
  <div class="gpu-card hover-scale">
    <div class="row">
      <div class="col-md-9">
        <h5><i class="bi bi-gpu-card me-2"></i>{{ gpu.name }}</h5>
        <p class="mb-1">状态: <span class="badge bg-success">{{ gpu.status || '可用' }}</span></p>
        <p class="mb-1">总内存: {{ gpu.memory_total }} MB</p>
        <p>可用内存: {{ gpu.memory_free }} MB ({{ memoryUsagePercent }}% 已使用)</p>
      </div>
      <div class="col-md-3 text-end">
        <div class="text-primary fw-bold">利用率: {{ gpu.utilization }}%</div>
      </div>
    </div>
    <div class="progress memory-bar mb-2">
      <div 
        class="progress-bar" 
        :class="memoryBarClass" 
        role="progressbar" 
        :style="{ width: memoryUsagePercent + '%' }"
        :aria-valuenow="memoryUsagePercent"
        aria-valuemin="0" 
        aria-valuemax="100"
      >{{ memoryUsagePercent }}%</div>
    </div>
    
    <div>
      <span class="me-2">GPU利用率:</span>
      <div class="progress">
        <div 
          class="progress-bar bg-info" 
          role="progressbar" 
          :style="{ width: gpu.utilization + '%' }"
          :aria-valuenow="gpu.utilization"
          aria-valuemin="0" 
          aria-valuemax="100"
        >{{ gpu.utilization }}%</div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'GpuCard',
  props: {
    gpu: {
      type: Object,
      required: true
    }
  },
  computed: {
    memoryUsagePercent() {
      // 计算内存使用百分比
      if (!this.gpu || !this.gpu.memory_total) return 0;
      const usedMemory = this.gpu.memory_total - this.gpu.memory_free;
      return Math.round((usedMemory / this.gpu.memory_total) * 100);
    },
    memoryBarClass() {
      // 根据内存使用率调整颜色
      const usage = this.memoryUsagePercent;
      if (usage < 50) return 'bg-info';
      if (usage < 80) return 'bg-warning';
      return 'bg-danger';
    }
  }
}
</script>

<style scoped>
.gpu-card {
  background-color: #fff;
  border-radius: 8px;
  padding: 15px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
  margin-bottom: 15px;
  transition: transform 0.3s ease;
}
.gpu-card:hover {
  transform: translateY(-5px);
}
.memory-bar {
  height: 20px;
}
</style> 