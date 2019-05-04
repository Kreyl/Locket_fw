#ifndef QHSM_H
#define QHSM_H

#include <stdint.h>

#define Q_MAX_DEPTH 8

typedef int QSignal;
typedef int QState;

typedef struct
{
    QSignal sig;
} QEvt;

typedef QState (*QStateHandler)(void *const me, const QEvt *const event);

enum
{
    QEP_EMPTY_SIG_ = 0,
    Q_ENTRY_SIG,
    Q_EXIT_SIG,
    Q_INIT_SIG,
    Q_USER_SIG,
};

enum
{
    Q_RET_SUPER,
    Q_RET_UNHANDLED,
    Q_RET_HANDLED,
    Q_RET_IGNORED,
    Q_RET_TRAN,
};

typedef struct
{
    QStateHandler current_;
    QStateHandler effective_;
    QStateHandler target_;
} QHsm;

#define Q_MSM_UPCAST(me) ((QHsm *)(me))
#define Q_STATE_CAST(handler) ((QStateHandler)(handler))

#define Q_UNHANDLED() ((QState)(Q_RET_UNHANDLED))
#define Q_HANDLED() ((QState)(Q_RET_HANDLED))
#define Q_TRAN(target) \
    ((Q_MSM_UPCAST(me))->target_ = Q_STATE_CAST(target), \
    (QState)(Q_RET_TRAN))
#define Q_SUPER(super) \
    ((Q_MSM_UPCAST(me))->effective_ = Q_STATE_CAST(super), \
    (QState)(Q_RET_SUPER))

#define QMSM_INIT(me, event) (QMsm_init(me, event))
#define QMSM_DISPATCH(me, event) (QMsm_dispatch(me, event)) 

#ifdef __cplusplus
extern "C" {
#endif

QState QHsm_top(void *const me, const QEvt *const event);

void QHsm_ctor(QHsm *const me, QStateHandler initial);

void QMsm_init(QHsm *me, const QEvt *const event);
QState QMsm_dispatch(QHsm *me, const QEvt *const event);

#ifdef __cplusplus
}
#endif

#endif
