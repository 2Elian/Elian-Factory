<template>
  <div class="container-fluid">
    <div class="row">
      <!-- 侧边栏 -->
      <SideBar 
        :activePage="activePage" 
        @change-page="changePage" 
      />

      <!-- 主内容区 -->
      <div class="col-md-10 main-content">
        <transition name="fade" mode="out-in">
          <component :is="currentComponent" />
        </transition>
      </div>
    </div>
  </div>
</template>

<script>
import SideBar from './components/SideBar.vue'
import TrainingView from './components/TrainingView.vue'
import InferenceView from './components/InferenceView.vue'
import ElianFactoryIntroView from './components/ElianFactoryIntroView.vue'

export default {
  name: 'App',
  components: {
    SideBar,
    TrainingView,
    InferenceView,
    ElianFactoryIntroView
  },
  data() {
    return {
      activePage: 'training',
      pageComponents: {
        training: TrainingView,
        inference: InferenceView,
        intro: ElianFactoryIntroView,
      }
    }
  },
  computed: {
    currentComponent() {
      return this.pageComponents[this.activePage]
    }
  },
  methods: {
    changePage(page) {
      this.activePage = page
    }
  }
}
</script>

<style>
/* 全局样式已在assets/styles.css中定义 */
</style> 