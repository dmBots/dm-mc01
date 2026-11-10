#ifndef BOARD_H
#define BOARD_h



#ifdef __cplusplus
extern "C" {
#endif


/* 芯片初始化成功后调用：打开外设 5 V，先关闭加热供电；具体测试还要由 main 启动。 */
void board_init();

#ifdef __cplusplus
}
#endif

#endif
