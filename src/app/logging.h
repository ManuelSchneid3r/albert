// Copyright (C) 2014-2021 Manuel Schneider
#pragma once
#include <QtConcurrent>
Q_DECLARE_LOGGING_CATEGORY(clc)
#define DEBG qCDebug(clc,).noquote()
#define INFO qCInfo(clc,).noquote()
#define WARN qCWarning(clc,).noquote()
#define CRIT qCCritical(clc,).noquote()
