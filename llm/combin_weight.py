#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2025/4/30 17:31
# @Author  : lizimo@nuist.edu.cn
# @File    : combin_weight.py
# @Description: 合并源模型权重和微调后的adapoter权重
from peft import PeftModel
from transformers import AutoModelForCausalLM
import torch
import os
from modelscope import AutoTokenizer
import shutil
import gc

def copy_files_not_in_B(A_path, B_path):
    """
    Copies files from directory A to directory B if they exist in A but not in B.

    :param A_path: Path to the source directory (A).
    :param B_path: Path to the destination directory (B).
    """
    # 保证路径存在
    if not os.path.exists(A_path):
        raise FileNotFoundError(f"The directory {A_path} does not exist.")
    if not os.path.exists(B_path):
        os.makedirs(B_path)

    # 获取路径A中所有非权重文件
    files_in_A = os.listdir(A_path)
    files_in_A = set([file for file in files_in_A if not (".bin" in file or "safetensors" in file)])
    # List all files in directory B
    files_in_B = set(os.listdir(B_path))

    # 找到所有A中存在但B中不存在的文件
    files_to_copy = files_in_A - files_in_B

    # 将文件或文件夹复制到B路径下
    for file in files_to_copy:
        src_path = os.path.join(A_path, file)
        dst_path = os.path.join(B_path, file)

        if os.path.isdir(src_path):
            # 复制目录及其内容
            shutil.copytree(src_path, dst_path)
        else:
            # 复制文件
            shutil.copy2(src_path, dst_path)

def merge_lora_to_base_model(model_name_or_path, adapter_name_or_path, save_path):
    # 清空GPU缓存
    torch.cuda.empty_cache()
    gc.collect()
    
    try:
        # 如果文件夹不存在，就创建
        if not os.path.exists(save_path):
            os.makedirs(save_path)
            
        # 确保使用CPU加载模型以节省显存
        tokenizer = AutoTokenizer.from_pretrained(model_name_or_path, trust_remote_code=True)
        
        print("⏳ 开始加载基础模型...")
        model = AutoModelForCausalLM.from_pretrained(
            model_name_or_path,
            trust_remote_code=True,
            low_cpu_mem_usage=True,
            torch_dtype=torch.float16,
            device_map="auto"  # 自动选择可用设备
        )
        
        print("⏳ 加载适配器权重...")
        model = PeftModel.from_pretrained(model, adapter_name_or_path, device_map="auto")
        
        print("⏳ 合并权重中...")
        merged_model = model.merge_and_unload()
        
        print("⏳ 保存合并后模型...")
        tokenizer.save_pretrained(save_path)
        merged_model.save_pretrained(save_path, safe_serialization=True)  # 使用安全序列化
        
        copy_files_not_in_B(model_name_or_path, save_path)
        print("✅ 合并完毕!@Elian")
        
    except Exception as e:
        print(f"❌ 合并过程中出错: {str(e)}")
        # 尝试使用CPU回退
        try:
            print("🔄 尝试使用CPU回退方案...")
            model = AutoModelForCausalLM.from_pretrained(
                model_name_or_path,
                trust_remote_code=True,
                low_cpu_mem_usage=True,
                torch_dtype=torch.float16,
                device_map="cpu"
            )
            model = PeftModel.from_pretrained(model, adapter_name_or_path, device_map="cpu")
            merged_model = model.merge_and_unload()
            tokenizer.save_pretrained(save_path)
            merged_model.save_pretrained(save_path, safe_serialization=True)
            copy_files_not_in_B(model_name_or_path, save_path)
            print("✅ CPU回退方案合并成功!@Elian")
        except Exception as cpu_e:
            print(f"❌ CPU回退方案失败: {str(cpu_e)}")
    finally:
        # 最终清理
        del model
        del merged_model
        torch.cuda.empty_cache()
        gc.collect()

if __name__ == '__main__':
    model_name_or_path = "./ckpt/deepseek14"
    adapter_name_or_path = "E:\lottery\Elian-Factory-main\llm\output\your_t123ask\lora"
    save_path = "E:\lottery\Elian-Factory-main\llm\output\your_t123ask\lora\weight"
    merge_lora_to_base_model(model_name_or_path,adapter_name_or_path,save_path)

