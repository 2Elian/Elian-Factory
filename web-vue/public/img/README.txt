如何添加自定义网站图标（Favicon）

要将您的照片设置为网站图标，请按照以下步骤操作：

1. 准备您的照片，建议使用正方形图片以获得最佳效果
2. 使用在线工具（如https://realfavicongenerator.net/）将您的照片转换为多种尺寸的图标
3. 将生成的图标文件放置在此目录（web-vue/public/img/）下，命名如下：
   - favicon.ico（主图标文件）
   - favicon-16x16.png（16x16像素图标）
   - favicon-32x32.png（32x32像素图标）
   - apple-touch-icon.png（苹果设备图标，180x180像素）

4. 重新构建前端项目：
   - 进入web-vue目录
   - 运行 npm run build
   - 将dist目录内容复制到web目录

5. 重启服务器

注意：浏览器可能会缓存图标，如果您更改图标后没有看到变化，请尝试清除浏览器缓存或使用隐私浏览模式查看。 