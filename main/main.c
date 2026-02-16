#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#define EC11_S1 GPIO_NUM_11
#define EC11_S2 GPIO_NUM_12

pcnt_unit_handle_t ec11_unit_hdl=NULL;//ec11单元句柄
pcnt_channel_handle_t ec11_channel_s1_hdl=NULL;//ec11通道句柄1
pcnt_channel_handle_t ec11_channel_s2_hdl=NULL;//ec11通道句柄2

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
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(ec11_channel_s1_hdl,PCNT_CHANNEL_EDGE_ACTION_INCREASE,PCNT_CHANNEL_EDGE_ACTION_DECREASE));//上升沿+1，下降沿-1
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(ec11_channel_s1_hdl,PCNT_CHANNEL_LEVEL_ACTION_INVERSE,PCNT_CHANNEL_LEVEL_ACTION_KEEP));//高电平翻转，低电平保持
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(ec11_channel_s2_hdl,PCNT_CHANNEL_EDGE_ACTION_INCREASE,PCNT_CHANNEL_EDGE_ACTION_DECREASE));//上升沿+1，下降沿-1
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(ec11_channel_s2_hdl,PCNT_CHANNEL_LEVEL_ACTION_KEEP,PCNT_CHANNEL_LEVEL_ACTION_INVERSE));//高电平保持，低电平翻转

    ESP_ERROR_CHECK(pcnt_unit_enable(ec11_unit_hdl));//使能pcnt单元
    ESP_ERROR_CHECK(pcnt_unit_clear_count(ec11_unit_hdl));//清除计数值
    ESP_ERROR_CHECK(pcnt_unit_start(ec11_unit_hdl));//启动pcnt单元
}

void app_main(void)
{
    pcnt_init();//pcnt初始化
    int pulse_count = 0;
    while(1)
    {
        ESP_ERROR_CHECK(pcnt_unit_get_count(ec11_unit_hdl, &pulse_count));//获取当前计数值
        ESP_LOGI("PCNT","当前计数值: %d", pulse_count);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
