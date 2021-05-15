// Copyright (C) 2014-2021 Manuel Schneider

#pragma once
#include <QtCharts>

class StatsWidget : public QChartView
{
    Q_OBJECT

public:
    explicit StatsWidget(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void updateChart();

    QChart *_chart;
    QDateTimeAxis *_dateTimeAxis;
    QValueAxis *_valueAxis_activations;
    QValueAxis *_valueAxis_cumsum;

};
