/*
 * File:   main.c
 * Author: frozenkota
 *
 * Created on 2015/03/21, 17:20
 * device: dsPIC30f2020
 * fileversion:1.0
 */

/* �V�X�e���N���b�N�ɂ��Ă�205p�Q��
 * FRC_PLL = 466MHz
 * FCY = 466 / 8 / 2 = 29.125MHz
*/

/****************************************
 *   �w�b�_�̃C���N���[�h�ƒ萔�̒�`
 ***************************************/
#define FCY 29125000UL

#include <libpic30.h>
#include "xc.h"
#include "dsp.h"

/****************************************
 *   �R���t�B�M�����[�V�����ݒ�
 ***************************************/
_FBS( BWRP_OFF & WR_PROT_BOOT_OFF & NO_BOOT_CODE);
//       (1)            (2)             (3)
// -- �u�[�g�Z�O�����g�̏������ݕی�ݒ�
// �������݉\�ɂ��� ...(1)
// �������݉\�ɂ��� ...(2)
// -- Boot Segment Program Flash Code Protection
//  No Boot Segment ...(3)

_FGS( GWRP_OFF & GSS_OFF & CODE_PROT_OFF);
//      (1)           (2)      (3)
// -- �Z�O�����g�S�ʂ̐ݒ�
// �Z�O�����g�S�ʂ̏������݂����� ...(1)
// -- General Segment Code Protection
// Disable ...(2)
// Disable ...(3)

_FOSCSEL( FRC_PLL);
//        (1)
// -- ���U���[�h
// ����RC���U (FRC_PLL) ...(1)

_FOSC( PRIOSC_OFF & HS_EC_DIS & OSC2_IO & FRC_HI_RANGE & CSW_FSCM_OFF)
//        (1)          (2)        (3)         (4)            (5)
// --�@�v���C�}���O�t�����U��
// �O�����U�͖��� ...(1)
// �O�����U�͖��� ...(2)
// -- OSCI/OSCO Pin Function
// OSCO�s���̓f�W�^��I/O�Ƃ��Ďg�p ...(3)
// -- ���g�������W�̐ݒ�
// FRC�n�C�����W�Ɏw�� ...(4)
// -- �N���b�N�̊Ď��Ɛ؂�ւ��ݒ�
// �؂�ւ��͖����Ƃ��� ...(5)

_FWDT( FWDTEN_OFF);
//        (1)
// -- �E�H�b�`�h�b�O�^�C�}�̐ݒ�
// ���� ...(1)

_FPOR( PWRT_2 );
//       (1)
// -- �p���[�I�����Z�b�g�ݒ�
// ���� ...(1)

_FICD( ICS_PGD );
//      (1)
// -- Comm Channel Select
// Use PGC/EMUC and PGD/EMUD ...(1)

/****************************************
 *   �@�@�@�@�v���g�^�C�v�錾
 ***************************************/
void init_adc(void);        // ADC����̏����ݒ�
void init_tmr3(void);       // TMR3����̏����ݒ�
void init_tmr2(void);       // TMR2����̏����ݒ�
void init_pic(void);        // ���I/O�̏����ݒ�
void init_interrupt(void); // �����ݏ����S�ʐݒ�
void init_pwm(void);        // PWM�S�ʂ̏����ݒ�
void print7seg(double data);   // 7�Z�O�ւ̃f�[�^���X�V

/****************************************
 *   �@�@�@�O���[�o���ϐ��̐錾
 ***************************************/
int nanaseg[] = { // 7�Z�O�̓_���p�^�[��(�^��C-551SR)
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
 *   �@�@�@�@�@���C���֐�
 ***************************************/
int main(void) {
    init_pic();              // ���o�̓|�[�g�̏����ݒ�
    init_tmr2();             // �^�C�}�Q�֌W�̏����ݒ�
    init_pwm();              // PWM�S�ʏ����ݒ�
    
    while(1){                // �C�x���g�����܂őҋ@
    }

    return 0;
}

/****************************************
 *   �@�@�@�@���[�U�[��`�֐�
 ***************************************/

void init_pic()
{
    //-- ���o�͐ݒ�
    TRISA  = 0x0000;
    TRISB  = 0x0000;
    TRISD  = 0x0000;
    TRISE  = 0x0000;
    TRISF  = 0x0000;

    //-- �o�̓|�[�g������
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
    T2CONbits.TON = 0;          // �^�C�}��~
    T2CONbits.TSIDL = 1;        // �A�C�h����������p��
    T2CONbits.TGATE = 0;        // �Q�[�g���䂵�Ȃ�
    T2CONbits.TCKPS = 0b00;     // �v���X�P�[���l1:1
    T2CONbits.TCS = 0;          // �N���b�N�͓����N���b�N
}

/***************************************************
 * �o�̓R���y�A���W���[����P��PWM���[�h�Ƃ��ē��삳��
 * �邽�߂̏����ݒ�
 *
 * OC2RS: duty0~100(%)��16bit�̒l�ŕ\�����l��������
 * PR2: PWM���g�������߂�ϐ��ƂȂ�
 *      500Hz�̗���ڂ���.
 * PWM���g�� = 1 / PWM���� = 1 / {(PR2 + 1) * (1 / FCY) * "TMR2�v���X�P�[���l"}
 * PR2 = FCY / ("TMR2�v���X�P�[���l" * "PWM��]���g")
 *     = 29.125MHz / (1 * 500Hz) = 58250
 ***************************************************/
void init_pwm()
{
    //-- �o�͔�r���W�X�^
    // OC2CON: �`���l���p�̐��䃌�W�X�^
    OC2CONbits.OCM= 0b110;      // �P��PWM���[�h�@�t�H���g�s������
    OC2CONbits.OCTSEL = 0;      // �^�C�}�[�Q����r���̃N���b�N�^�C�}�[
    OC2CONbits.OCFLT = 0;       // PWM FAULT �����̔����Ȃ�(OCM=111�̏ꍇ�̂ݎg�p)
    OC2CONbits.OCSIDL = 0;      // �o�͔�rx��CPU IDLE���[�h�ł�����p��

    OC2RS = (int)(duty * 65535 / 100); // �f���[�e�B�[�� duty�͏����l50��
                                       // duty=0->OC2RS=0,duty=100->OC2RS=100
    PR2 = 58250;                // 500Hz
    T2CONbits.TON = 1;          // PWM�@�J�n
}
