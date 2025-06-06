#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2025/4/30 15:52
# @Author  : lizimo@nuist.edu.cn
# @File    : main.py
# @Description: transformers训练版本
import argparse
import sys
import json
from os.path import join
import pandas as pd
from datasets import Dataset
from transformers import (
    DataCollatorForSeq2Seq,
    AutoTokenizer,
)
import os
from models.transformers_model import load_model
from mydataloader.loader_transformers import process_data
from combin_weight import merge_lora_to_base_model
import warnings
warnings.simplefilter("ignore")
warnings.filterwarnings("ignore", category=FutureWarning)
warnings.filterwarnings("ignore", category=UserWarning)

def configuration_parameter():
    parser = argparse.ArgumentParser(description="LoRA fine-tuning for deepseek-r1 model @Elian")
    parser.add_argument('--config', type=str, help='配置文件路径')

    # 模型路径相关参数
    parser.add_argument("--model_name_or_path", type=str, default="./ckpt/qwen",
                        help="Path to the model directory downloaded locally")
    parser.add_argument("--output_dir", type=str,
                        default="./output/task_name/lora",
                        help="Directory to save the fine-tuned model and checkpoints")

    # 数据集路径
    parser.add_argument("--train_file", type=str, default="./data/shuangseqiu_train.jsonl",
                        help="Path to the training data file in JSONL format")

    # 训练超参数
    parser.add_argument("--num_train_epochs", type=int, default=1,
                        help="Number of training epochs")
    parser.add_argument("--per_device_train_batch_size", type=int, default=1,
                        help="Batch size per device during training")
    parser.add_argument("--gradient_accumulation_steps", type=int, default=2,
                        help="Number of updates steps to accumulate before performing a backward/update pass")
    parser.add_argument("--learning_rate", type=float, default=2e-4,
                        help="Learning rate for the optimizer")
    parser.add_argument("--max_seq_length", type=int, default=1024,
                        help="Maximum sequence length for the input")
    parser.add_argument("--logging_steps", type=int, default=1,
                        help="Number of steps between logging metrics")
    parser.add_argument("--save_steps", type=int, default=200,
                        help="Number of steps between saving checkpoints")
    parser.add_argument("--save_total_limit", type=int, default=1,
                        help="Maximum number of checkpoints to keep")
    parser.add_argument("--lr_scheduler_type", type=str, default="constant_with_warmup",
                        help="Type of learning rate scheduler")
    parser.add_argument("--warmup_steps", type=int, default=28,
                        help="Number of warmup steps for learning rate scheduler")

    # LoRA 特定参数
    parser.add_argument("--lora_rank", type=int, default=2,
                        help="Rank of LoRA matrices")
    parser.add_argument("--lora_alpha", type=int, default=4,
                        help="Alpha parameter for LoRA")
    parser.add_argument("--lora_dropout", type=float, default=0.05,
                        help="Dropout rate for LoRA")

    # 分布式训练参数
    parser.add_argument("--local_rank", type=int, default=int(os.environ.get("LOCAL_RANK", -1)),
                        help="Local rank for distributed training")
    parser.add_argument("--distributed", action="store_true", default=False, help="Enable distributed training")

    # 额外优化和硬件相关参数
    parser.add_argument("--gradient_checkpointing", action="store_true", default=True,
                    help="Enable gradient checkpointing to save memory")
    parser.add_argument("--optim", type=str, default="adamw_torch",
                        help="Optimizer to use during training")
    parser.add_argument("--train_mode", type=str, default="lora",
                        help="lora or qlora")
    parser.add_argument("--seed", type=int, default=42,
                        help="Random seed for reproducibility")
    parser.add_argument("--fp16", action="store_true", default=False,
                        help="Use mixed precision (FP16) training 我们这里使用bfloat16训练")
    parser.add_argument("--report_to", type=str, default=None,
                        help="Reporting tool for logging (e.g., tensorboard)")
    parser.add_argument("--dataloader_num_workers", type=int, default=0,
                        help="Number of workers for data loading")
    parser.add_argument("--save_strategy", type=str, default="steps",
                        help="Strategy for saving checkpoints ('steps', 'epoch')")
    parser.add_argument("--weight_decay", type=float, default=0,
                        help="Weight decay for the optimizer")
    parser.add_argument("--max_grad_norm", type=float, default=1,
                        help="Maximum gradient norm for clipping")
    parser.add_argument("--remove_unused_columns", action="store_true", default=True,
                        help="Remove unused columns from the dataset")
    args, remaining_argv = parser.parse_known_args()
    if args.config:
        try:
            with open(args.config, 'r', encoding='utf-8') as f:
                config_dict = json.load(f)
            for key, value in config_dict.items():
                # 跳过config参数本身
                if key == 'config':
                    continue
                if key in ['distributed', 'gradient_checkpointing', 'fp16']:
                    # 对于store_true参数，设置布尔值
                    setattr(args, key, value)
                else:
                    setattr(args, key, value)
                    
        except Exception as e:
            print(f"加载配置文件时出错: {e}")
            exit(1)
    with open("C:\Windows\elianfactory\default_config.json", 'r', encoding='utf-8') as f:
        config_dict = json.load(f)
    for key, value in config_dict.items():
        # 跳过config参数本身
        if key == 'config':
            continue
        if key in ['distributed', 'gradient_checkpointing', 'fp16']:
            # 对于store_true参数，设置布尔值
            setattr(args, key, value)
        else:
            setattr(args, key, value)
    
    args = parser.parse_args(remaining_argv, namespace=args)
    
    return args



# 训练部分
def main():
    args = configuration_parameter()
    if args.distributed:
        os.environ["TORCH_DISTRIBUTED_BACKEND"] = "gloo"
        os.environ["MASTER_ADDR"] = "127.0.0.1"
        os.environ["MASTER_PORT"] = "29500"
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)
    print("🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟Elian-Factory开始训练，训练配置参数如下:🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟")
    for key, value in vars(args).items():
        print(f"  {key}: {value}")
    print("🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟加载分词器@Elian🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟")
    # 加载分词器
    model_path = args.model_name_or_path
    tokenizer = AutoTokenizer.from_pretrained(model_path, trust_remote_code=True, use_fast=False)
    print("加载完毕!~...@Elian")
    print("🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟处理数据@Elian🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟")
    # 处理数据
    data = pd.read_json(args.train_file, lines=True)
    train_ds = Dataset.from_pandas(data)
    train_dataset = train_ds.map(process_data,
                                 fn_kwargs={"tokenizer": tokenizer, "max_seq_length": args.max_seq_length},
                                 remove_columns=train_ds.column_names)
    print(f"☀️☀️☀️训练数据共{len(train_dataset)}条☀️☀️☀️")
    data_collator = DataCollatorForSeq2Seq(tokenizer=tokenizer, padding=True, return_tensors="pt")
    print("🌟🌟数据集处理完毕!~...@Elian")
    # 加载模型
    print("🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟训练@Elian🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟🌟")
    trainer = load_model(args, train_dataset, data_collator)
    print("🌟🌟开始训练!~...@Elian")
    trainer.train()
    # 训练
    final_save_path = join(args.output_dir)
    trainer.save_model(final_save_path)
    if int(os.environ.get("LOCAL_RANK", "0")) == 0:
        combin_sava_path = os.path.join(args.output_dir, 'weight')
        print(f"🌟🌟训练完毕!开始合并权重文件到{combin_sava_path}...@Elian")
        merge_lora_to_base_model(args.model_name_or_path, args.output_dir, combin_sava_path)
    if args.distributed:
        import torch.distributed as dist
        dist.barrier()

if __name__ == "__main__":
    main()