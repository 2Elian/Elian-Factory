const { defineConfig } = require('@vue/cli-service')

module.exports = defineConfig({
  transpileDependencies: true,
  // 配置开发服务器代理，连接到后端API
  devServer: {
    proxy: {
      '/api': {
        target: 'http://localhost:10171',
        changeOrigin: true
      }
    }
  },
  // 配置构建输出目录为项目根目录下的web文件夹
  outputDir: '../web',
  // 生产环境不生成sourceMap
  productionSourceMap: false
}) 