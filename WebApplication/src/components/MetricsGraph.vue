<script>
import { ref, shallowRef, toRaw } from 'vue';
import { Chart, LineController, LineElement, LinearScale, CategoryScale, PointElement, Tooltip } from 'chart.js';
import { mapState } from "vuex"
import dateHelpers from '../helpers/date-helpers';
import { right } from '@popperjs/core';

// Register ChartJS components
Chart.register([LineController, LineElement, LinearScale, CategoryScale, PointElement, Tooltip]);

export default {
    props: ["metricsName"],
    emits: [],
    data() {
        return {
            chartInstance: null
        };
    },
    computed: {
        ...mapState({
        }),
    },
    async mounted() {
        this.renderGraph();
    },
    async created() {
        this.messageBus.on('refresh-metrics-' + this.metricsName, this.renderGraph);
    },
    async unmounted() {
        if (this.chartInstance) {
            this.chartInstance.destroy();
            this.chartInstance = null;
        }
    },
    methods: {
        renderGraph() {
            console.log("rendering graph");
            const data2 = {
                datasets: 
                [
                    {
                        data: document.metricsHistory[this.metricsName],
                        backgroundColor: '#FF0000',
                        pointRadius: 0,
                        borderRadius: 0,
                        borderColor: '#00FF00'
                    } ]
                }

            const options = {
                responsive: true,
                maintainAspectRatio: false,
                clip: false,
                animations: false,
                scales: {
                    x: {
                        type: 'linear',
                        title: { display: false, text: 'Time (s)' },
                        display: false,
                        min: document.metricsLowerTimestamp,
                        max: document.metricsHigherTimestamp
                    },
                    y: {
                        type: 'linear',
                        title: { display: false, text: 'Value' },
                        // min: 0,
                        display: false
                    },
                },
                plugins: {
                    legend: { display: false },
                    tooltip: {
                        intersect: false,
                        xAlign: 'right',
                        yAlign: 'bottom',
                        displayColors: false,
                        callbacks: {
                            title: function(context) { return null;},
                            label: function(context) { 
                                return dateHelpers.getElapsedDurationFromMilliseconds((document.metricsHigherTimestamp - context.parsed.x) / 1000) + ": " +context.parsed.y;
                            }
                        }
                    },
                },
            };

            let chartCanvas = this.$refs['canvas-graph'];
            const ctx = chartCanvas.getContext('2d');
            if (this.chartInstance) {
                this.chartInstance.destroy(); // Destroy previous instance
            }
            this.chartInstance = new Chart(ctx, {
                type: 'line',
                data: data2,
                options: options,
            });
        }
    }
}
</script>

<template>
    <div style="width: 100%; height: 30px;">
        <canvas style="width: 100%; height: 100%;" ref="canvas-graph"></canvas>
    </div>
</template>