<template>
  <div class="col-md-2 sidebar">
    <div class="d-flex flex-column p-3 h-100">
      <div class="text-center mb-4">
        <h3 class="text-white">🐤ElianFactory</h3>
        <h5 class="text-white">Windows的LLM训练平台</h5>
        <p class="text-light">版本: V1.0</p>
      </div>
      <hr class="text-white opacity-25">
      <ul class="nav nav-pills flex-column mb-auto">
        <li class="nav-item" v-for="(item, key) in menuItems" :key="key">
          <a 
            href="#" 
            :class="['nav-link', activePage === key ? 'active' : 'text-white']"
            @click.prevent="changePage(key)"
          >
            <i :class="['bi', item.icon, 'me-2']"></i>{{ item.title }}
          </a>
        </li>
      </ul>
      <hr class="text-white opacity-25">
      <div class="text-center text-white small">
        <p>基于Transformers开发</p>
        <p>作者：Elian | <a href="https://github.com/ElianFoundation" target="_blank" class="text-white">点亮本项目的stars<i class="bi bi-github ms-1"></i></a></p>
        <p>© 2025 Elian All Rights Reserved</p>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'SideBar',
  props: {
    activePage: {
      type: String,
      required: true
    }
  },
  data() {
    return {
      menuItems: {
        training: { title: '模型训练', icon: 'bi-cpu' },
        inference: { title: '模型推理', icon: 'bi-chat-dots' },
        ollama: { title: '部署到Ollama', icon: 'bi-box-arrow-up' },
        intro: { title: '关于ElianFactory', icon: 'bi-info-circle' }
      }
    }
  },
  methods: {
    changePage(page) {
      this.$emit('change-page', page)
    }
  }
}
</script>

<style scoped>
/* 特定组件样式可以在这里添加 */
</style> 