#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2025/5/3 14:25
# @Author  : lizimo@nuist.edu.cn
# @File    : transformers_model.py 模型训练
# @Description:
from loguru import logger
from transformers import (
    TrainingArguments,
    AutoModelForCausalLM,
    Trainer,
)
import torch
import swanlab
from peft import LoraConfig, get_peft_model, TaskType
import bitsandbytes as bnb
from torch import nn
from torch.nn.parallel import DistributedDataParallel as DDP
import torch.distributed as dist
swanlab.login(api_key="SzlnCgFuU6SBHi7MvE74n")
def find_all_linear_names(model, train_mode):
    """
    找出所有全连接层，为所有全连接添加adapter
    """
    assert train_mode in ['lora', 'qlora']
    cls = bnb.nn.Linear4bit if train_mode == 'qlora' else nn.Linear
    lora_module_names = set()
    for name, module in model.named_modules():
        if isinstance(module, cls):
            names = name.split('.')
            lora_module_names.add(names[0] if len(names) == 1 else names[-1])

    if 'lm_head' in lora_module_names:  # needed for 16-bit
        lora_module_names.remove('lm_head')
    lora_module_names = list(lora_module_names)
    logger.info(f'LoRA target module names: {lora_module_names}')
    return lora_module_names


def setup_distributed(args):
    """初始化分布式环境"""
    if args.distributed:
        if args.local_rank == -1:
            raise ValueError("未正确初始化 local_rank，请确保通过分布式启动脚本传递参数 --> 例如:torchrun")

        # 初始化分布式进程组
        dist.init_process_group(backend="gloo")
        torch.cuda.set_device(args.local_rank)
        print(f"分布式训练已启用Local rank: {args.local_rank}@Elian")
    else:
        print("未启用分布式训练 --> 单线程模式@Elian")

def load_model(args, train_dataset, data_collator):
    # 初始化分布式环境
    setup_distributed(args)
    # 加载模型
    model_kwargs = {
        "trust_remote_code": True,
        "torch_dtype": torch.float16 if args.fp16 else torch.bfloat16,
        "use_cache": False if args.gradient_checkpointing else True,
        "device_map": "cuda:0" if not args.distributed else None,
        "load_in_4bit": True
    }
    model = AutoModelForCausalLM.from_pretrained(args.model_name_or_path, **model_kwargs)
    # 模型的词嵌入层参与训练
    model.enable_input_require_grads()

    # 哪些模块需要注入Lora参数
    target_modules = find_all_linear_names(model.module if isinstance(model, DDP) else model, args.train_mode)
    # lora配置
    config = LoraConfig(
        r=args.lora_rank,
        lora_alpha=args.lora_alpha,
        lora_dropout=args.lora_dropout,
        bias="none",
        target_modules=target_modules,
        task_type=TaskType.CAUSAL_LM,
        inference_mode=False

    )
    use_bfloat16 = torch.cuda.is_bf16_supported()  # 检查设备是否支持 bf16
    # 配置训练参数
    if args.distributed:
        train_args = TrainingArguments(
        output_dir=args.output_dir,
        per_device_train_batch_size=args.per_device_train_batch_size,
        gradient_accumulation_steps=args.gradient_accumulation_steps,
        logging_steps=args.logging_steps,
        num_train_epochs=args.num_train_epochs,
        save_steps=args.save_steps,
        learning_rate=args.learning_rate,
        save_on_each_node=True,
        report_to=args.report_to,
        seed=args.seed,
        optim=args.optim,
        local_rank=args.local_rank if args.distributed else -1,
        ddp_find_unused_parameters=False,  # 分布式参数检查优化
        fp16=args.fp16,
        bf16=not args.fp16 and use_bfloat16,
        remove_unused_columns=False
    )
    else:
        train_args = TrainingArguments(
        output_dir=args.output_dir,
        per_device_train_batch_size=args.per_device_train_batch_size,
        gradient_accumulation_steps=args.gradient_accumulation_steps,
        logging_steps=args.logging_steps,
        num_train_epochs=args.num_train_epochs,
        save_steps=args.save_steps,
        learning_rate=args.learning_rate,
        save_on_each_node=True,
        gradient_checkpointing=args.gradient_checkpointing,
        report_to=args.report_to,
        seed=args.seed,
        optim=args.optim,
        local_rank=args.local_rank if args.distributed else -1,
        ddp_find_unused_parameters=False,  # 分布式参数检查优化
        fp16=args.fp16,
        bf16=not args.fp16 and use_bfloat16,
        remove_unused_columns=False
    )
    # 应用 PEFT 配置到模型
    model = get_peft_model(model.module if isinstance(model, DDP) else model, config)
    # 将模型移动到分布式设备
    if args.distributed:
        model.to(args.local_rank)
        model = DDP(model, device_ids=[args.local_rank], output_device=args.local_rank)
        model.module.gradient_checkpointing_enable()
    if args.local_rank == 0:
        print("❄️❄️❄️可训练参数和总参数如下：❄️❄️❄️")
        if args.distributed:
            model.module.print_trainable_parameters()
        else:
            model.print_trainable_parameters()
    trainer = Trainer(
        model=model,
        args=train_args,
        train_dataset=train_dataset,
        data_collator=data_collator
    )
    return trainer