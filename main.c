/*
 * File:   main.c
 * Author: frozenkota
 *
 * Created on 2015/03/21, 17:20
 * device: dsPIC30f2020
 * fileversion:1.0
 */

/* システムクロックについては205p参照
 * FRC_PLL = 466MHz
 * FCY = 466 / 8 / 2 = 29.125MHz
*/

/****************************************
 *   ヘッダのインクルードと定数の定義
 ***************************************/
#define FCY 29125000UL

#include <libpic30.h>
#include "xc.h"
#include "dsp.h"

/****************************************
 *   コンフィギュレーション設定
 ***************************************/
_FBS( BWRP_OFF & WR_PROT_BOOT_OFF & NO_BOOT_CODE);
//       (1)            (2)             (3)
// -- ブートセグメントの書き込み保護設定
// 書き込み可能にする ...(1)
// 書き込み可能にする ...(2)
// -- Boot Segment Program Flash Code Protection
//  No Boot Segment ...(3)

_FGS( GWRP_OFF & GSS_OFF & CODE_PROT_OFF);
//      (1)           (2)      (3)
// -- セグメント全般の設定
// セグメント全般の書き込みを許可 ...(1)
// -- General Segment Code Protection
// Disable ...(2)
// Disable ...(3)

_FOSCSEL( FRC_PLL);
//        (1)
// -- 発振モード
// 内蔵RC発振 (FRC_PLL) ...(1)

_FOSC( PRIOSC_OFF & HS_EC_DIS & OSC2_IO & FRC_HI_RANGE & CSW_FSCM_OFF)
//        (1)          (2)        (3)         (4)            (5)
// --　プライマリ外付け発振源
// 外部発振は無効 ...(1)
// 外部発振は無効 ...(2)
// -- OSCI/OSCO Pin Function
// OSCOピンはデジタルI/Oとして使用 ...(3)
// -- 周波数レンジの設定
// FRCハイレンジに指定 ...(4)
// -- クロックの監視と切り替え設定
// 切り替えは無効とする ...(5)

_FWDT( FWDTEN_OFF);
//        (1)
// -- ウォッチドッグタイマの設定
// 無効 ...(1)

_FPOR( PWRT_2 );
//       (1)
// -- パワーオンリセット設定
// 無効 ...(1)

_FICD( ICS_PGD );
//      (1)
// -- Comm Channel Select
// Use PGC/EMUC and PGD/EMUD ...(1)

/****************************************
 *   　　　　プロトタイプ宣言
 ***************************************/
void init_adc(void);        // ADC周りの初期設定
void init_tmr3(void);       // TMR3周りの初期設定
void init_tmr2(void);       // TMR2周りの初期設定
void init_pic(void);        // 主にI/Oの初期設定
void init_interrupt(void); // 割込み処理全般設定
void init_pwm(void);        // PWM全般の初期設定
void print7seg(double data);   // 7セグへのデータを更新

/****************************************
 *   　　　グローバル変数の宣言
 ***************************************/
int nanaseg[] = { // 7セグの点灯パターン(型番C-551SR)
    0b01110111,   // 0
    0b00010100,   // 1
    0b10110011,   // 2
    0b10110110,   // 3
    0b11010100,   // 4
    0b11100110,   // 5
    0b11000111,   // 6
    0b00110100,   // 7
    0b11110111,   // 8
    0b11110100    // 9
};
double duty = 50.0;

/****************************************
 *   　　　　　メイン関数
 ***************************************/
int main(void) {
    init_pic();              // 入出力ポートの初期設定
    init_tmr2();             // タイマ２関係の初期設定
    init_pwm();              // PWM全般初期設定
    
    while(1){                // イベント発生まで待機
    }

    return 0;
}

/****************************************
 *   　　　　ユーザー定義関数
 ***************************************/

void init_pic()
{
    //-- 入出力設定
    TRISA  = 0x0000;
    TRISB  = 0x0000;
    TRISD  = 0x0000;
    TRISE  = 0x0000;
    TRISF  = 0x0000;

    //-- 出力ポート初期化
    PORTA  = 0x0000;
    PORTB  = 0x0000;
    PORTD  = 0x0000;
    PORTE  = 0x0000;
    PORTF  = 0x0000;
    LATA   = 0x0000;
    LATB   = 0x0000;
    LATD   = 0x0000;
    LATE   = 0x0000;
    LATF   = 0x0000;
}

void init_tmr2()
{
    T2CONbits.TON = 0;          // タイマ停止
    T2CONbits.TSIDL = 1;        // アイドル中も動作継続
    T2CONbits.TGATE = 0;        // ゲート制御しない
    T2CONbits.TCKPS = 0b00;     // プリスケール値1:1
    T2CONbits.TCS = 0;          // クロックは内部クロック
}

/***************************************************
 * 出力コンペアモジュールを単純PWMモードとして動作させ
 * るための初期設定
 *
 * OC2RS: duty0~100(%)を16bitの値で表した値を代入する
 * PR2: PWM周波数を決める変数となる
 *      500Hzの例を載せる.
 * PWM周波数 = 1 / PWM周期 = 1 / {(PR2 + 1) * (1 / FCY) * "TMR2プリスケーラ値"}
 * PR2 = FCY / ("TMR2プリスケーラ値" * "PWM希望周波")
 *     = 29.125MHz / (1 * 500Hz) = 58250
 ***************************************************/
void init_pwm()
{
    //-- 出力比較レジスタ
    // OC2CON: チャネル用の制御レジスタ
    OC2CONbits.OCM= 0b110;      // 単純PWMモード　フォルトピン無効
    OC2CONbits.OCTSEL = 0;      // タイマー２が比較ｘのクロックタイマー
    OC2CONbits.OCFLT = 0;       // PWM FAULT 条件の発生なし(OCM=111の場合のみ使用)
    OC2CONbits.OCSIDL = 0;      // 出力比較xはCPU IDLEモードでも動作継続

    OC2RS = (int)(duty * 65535 / 100); // デューティー比 dutyは初期値50％
                                       // duty=0->OC2RS=0,duty=100->OC2RS=100
    PR2 = 58250;                // 500Hz
    T2CONbits.TON = 1;          // PWM　開始
}
