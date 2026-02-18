#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio_filter.h"
#define EC11_button GPIO_NUM_10
#define EC11_S1 GPIO_NUM_11
#define EC11_S2 GPIO_NUM_12

pcnt_unit_handle_t ec11_unit_hdl=NULL;//ec11单元句柄
pcnt_channel_handle_t ec11_channel_s1_hdl=NULL;//ec11通道句柄1
pcnt_channel_handle_t ec11_channel_s2_hdl=NULL;//ec11通道句柄2
gpio_glitch_filter_handle_t ec11_button_filter_hdl=NULL;//ec11按键毛刺过滤器句柄
static volatile bool ec11_button=false;//全局标志位记录按键状态 volatile确保isr修改变量能被main正确读取
//s1通道：上升沿+1，下降沿-1，高电平翻转，低电平保持
//s2通道：上升沿+1，下降沿-1，高电平保持，低电平翻转
//每次旋转s1、s2各接受一个上升沿一个下降沿，计数单元共计+4或-4
void pcnt_init()//pcnt初始化
{
    pcnt_unit_config_t ec11_unit_cfg={//配置pcnt单元
        .low_limit=-100,//最小计数值
        .high_limit=100,//最大计数值
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&ec11_unit_cfg, &ec11_unit_hdl));//新建pcnt单元并判断是否成功
    
    pcnt_chan_config_t ec11_channel_s1_cfg={//配置pcnt通道1
        .edge_gpio_num=EC11_S1,//输入s1边沿
        .level_gpio_num=EC11_S2,//输入s2电平
    };
    ESP_ERROR_CHECK(pcnt_new_channel(ec11_unit_hdl,&ec11_channel_s1_cfg,&ec11_channel_s1_hdl));//新建pcnt通道1并判断是否成功
    
    pcnt_chan_config_t ec11_channel_s2_cfg={//配置pcnt通道2
        .edge_gpio_num=EC11_S2,//输入s2边沿
        .level_gpio_num=EC11_S1,//输入s1电平
    };
    ESP_ERROR_CHECK(pcnt_new_channel(ec11_unit_hdl,&ec11_channel_s2_cfg,&ec11_channel_s2_hdl));//新建pcnt通道2并判断是否成功
    
    pcnt_glitch_filter_config_t filte_cfg={//配置毛刺过滤器
        .max_glitch_ns=1000,//过滤1000ns以下毛刺
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(ec11_unit_hdl, &filte_cfg));//新建毛刺过滤器并判断是否成功
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(ec11_channel_s1_hdl,
                                                PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                                PCNT_CHANNEL_EDGE_ACTION_DECREASE));//上升沿+1，下降沿-1
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(ec11_channel_s1_hdl,
                                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
                                                PCNT_CHANNEL_LEVEL_ACTION_KEEP));//高电平翻转，低电平保持
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(ec11_channel_s2_hdl,
                                                PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                                PCNT_CHANNEL_EDGE_ACTION_DECREASE));//上升沿+1，下降沿-1
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(ec11_channel_s2_hdl,
                                                PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));//高电平保持，低电平翻转

    ESP_ERROR_CHECK(pcnt_unit_enable(ec11_unit_hdl));//使能pcnt单元
    ESP_ERROR_CHECK(pcnt_unit_clear_count(ec11_unit_hdl));//清除计数值
    ESP_ERROR_CHECK(pcnt_unit_start(ec11_unit_hdl));//启动pcnt单元
}

void IRAM_ATTR ec11_button_isr_hdl(void *arg)//ec11按键中断处理函数
{
    ec11_button=true;//翻转按键状态
}

void button_init()//ec11按键初始化：下降沿触发防止按下后反复触发
{
    gpio_config_t gpio_cfg={
        .pin_bit_mask=(1ULL<<EC11_button),
        .mode=GPIO_MODE_INPUT,//输入模式
        .pull_up_en=GPIO_PULLUP_ENABLE,//开启上拉
        .pull_down_en=GPIO_PULLDOWN_DISABLE,//不开启下拉
        .intr_type=GPIO_INTR_NEGEDGE,//开启中断，下降沿触发
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));//配置GPIO并判断是否成功
    ESP_ERROR_CHECK(gpio_install_isr_service(0));//安装GPIO中断，中断服务使用默认优先级0
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_button, ec11_button_isr_hdl, NULL));//给按钮引脚添加GPIO中断处理函数，不传入参数

    gpio_pin_glitch_filter_config_t ec11_button_fliter_cfg={//配置ec11按键毛刺过滤器
        .gpio_num=EC11_button,
    };
    ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(&ec11_button_fliter_cfg, &ec11_button_filter_hdl));//新建毛刺过滤器并判断是否成功
    ESP_ERROR_CHECK(gpio_glitch_filter_enable(ec11_button_filter_hdl));//使能毛刺过滤器
}

void app_main(void)
{
    pcnt_init();//pcnt初始化
    button_init();//按键初始化
    int pulse_count = 0;
    while(1)
    {
        if (ec11_button==true)
        {
            ESP_ERROR_CHECK(pcnt_unit_get_count(ec11_unit_hdl, &pulse_count));//获取当前计数值
            ESP_LOGI("PCNT","当前计数值: %d", pulse_count);
            ESP_ERROR_CHECK(pcnt_unit_clear_count(ec11_unit_hdl));//清除计数值
            ESP_LOGI("PCNT","计数已清零");
            ec11_button=false;//清零按键状态标志位
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
