<script>
import axios from "axios"
import api from "../orthancApi"
import SeriesItem from "./SeriesItem.vue"
import { translateDicomTag } from "../locales/i18n"

export default {
    props: ['studyId', 'patientMainDicomTags', 'studyMainDicomTags', 'studySeries'],
    emits: ['deletedStudy'],
    data() {
        return {
            seriesInfo: {},
        };
    },
    created() {
        // Инициализируем seriesInfo сразу при создании компонента
        this.initSeriesInfo();
    },
    computed: {
        sortedSeriesIds() {
            let keys = Object.keys(this.seriesInfo);
            keys.sort((a, b) => {
                const numA = parseInt(this.seriesInfo[a]?.MainDicomTags?.SeriesNumber) || 0;
                const numB = parseInt(this.seriesInfo[b]?.MainDicomTags?.SeriesNumber) || 0;
                return numA - numB;
            });
            return keys;
        }
    },
    watch: {
        studySeries: {
            handler(newValue) {
                this.initSeriesInfo();
            },
            deep: true
        }
    },
    methods: {
        initSeriesInfo() {
            if (!this.studySeries || this.studySeries.length === 0) return;

            const newInfo = {};
            for (const series of this.studySeries) {
                if (series && series.ID) {
                    newInfo[series.ID] = series;
                }
            }
            this.seriesInfo = newInfo;
        },
        columnTitle(tagName) {
            if (tagName == "instances_number") {
                return "# " + this.$i18n.t('instances');
            } else {
                return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
            }
        },
        columnTooltip(tagName) {
            if (tagName == "instances_number") {
                return this.$i18n.t("instances_number");
            } else {
                return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
            }
        },
        onDeletedSeries(seriesId) {
            delete this.seriesInfo[seriesId];
            if (Object.keys(this.seriesInfo).length == 0) {
                this.$emit("deletedStudy", this.studyId);
            } else {
                this.messageBus.emit('deleted-series-from-study-' + this.studyId);
            }
        }
    },
    components: { SeriesItem }
}
</script>

<template>
    <table v-if="sortedSeriesIds.length > 0" class="table table-responsive table-sm series-table">
        <thead>
            <tr>
                <th width="2%" scope="col" class="series-table-header"></th>
                <th width="7%" scope="col" class="series-table-header cut-text" data-bs-toggle="tooltip"
                    :title="columnTooltip('SeriesNumber')">{{ columnTitle('SeriesNumber') }}</th>
                <th width="40%" scope="col" class="series-table-header cut-text" data-bs-toggle="tooltip"
                    :title="columnTooltip('SeriesDescription')">{{ columnTitle('SeriesDescription') }}</th>
                <th width="11%" scope="col" class="series-table-header cut-text text-center" data-bs-toggle="tooltip"
                    :title="columnTooltip('Modality')">{{ columnTitle('Modality') }}</th>
                <th width="5%" scope="col" class="series-table-header cut-text text-center" data-bs-toggle="tooltip"
                    :title="columnTooltip('instances_number')">{{ columnTitle('instances_number') }}</th>
            </tr>
        </thead>
        <SeriesItem v-for="seriesId in sortedSeriesIds" :key="seriesId" :seriesId="seriesId"
            :seriesInfo="seriesInfo[seriesId]" :studyMainDicomTags="studyMainDicomTags"
            :patientMainDicomTags="patientMainDicomTags" @deletedSeries="onDeletedSeries" />
    </table>
</template>

<style>
/* =============================================================================
   RESOURCEBUTTONGROUP.VUE - MODERN STYLES (D-MIS Design System)
   
   Современные стили для кнопок управления ресурсами
   ============================================================================= */

/* =============================================================================
   CONTAINER LAYOUT
   ============================================================================= */

/* Основной контейнер для кнопок */
.resource-button-group,
.study-button-group {
    display: flex;
    flex-direction: column;
    gap: 12px;
    align-items: flex-end;
    padding: 0;
    min-width: 60px;
}

/* Horizontal layout */
.resource-button-group.horizontal,
.study-button-group.horizontal {
    flex-direction: row;
    flex-wrap: wrap;
    justify-content: flex-end;
    align-items: center;
}

/* =============================================================================
   BASE BUTTON RESET & COMMON STYLES
   ============================================================================= */

.resource-button-group button,
.resource-button-group a,
.resource-button-group .btn {
    all: unset;
    box-sizing: border-box;
    cursor: pointer;
    font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
    transition: all 0.2s ease;
    user-select: none;
}

/* =============================================================================
   ICON-ONLY BUTTONS (основной стиль для вертикального ряда)
   ============================================================================= */

.resource-button-group button,
.resource-button-group a,
.resource-button-group .btn {
    width: 48px;
    height: 48px;
    min-width: 48px;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(135deg, #64748b 0%, #475569 100%);
    border: none;
    border-radius: 10px;
    color: white;
    font-size: 20px;
    box-shadow: 0 2px 6px rgba(100, 116, 139, 0.25);
    position: relative;
}

.resource-button-group button:hover,
.resource-button-group a:hover,
.resource-button-group .btn:hover {
    background: linear-gradient(135deg, #475569 0%, #334155 100%);
    box-shadow: 0 4px 12px rgba(100, 116, 139, 0.35);
    transform: scale(1.08);
}

.resource-button-group button:active,
.resource-button-group a:active,
.resource-button-group .btn:active {
    transform: scale(1.03);
    box-shadow: 0 2px 4px rgba(100, 116, 139, 0.2);
}

/* Icon inside button */
.resource-button-group button i,
.resource-button-group a i,
.resource-button-group .btn i {
    font-size: 20px;
    color: white;
    line-height: 1;
    transition: none;
}

/* =============================================================================
   COLOR VARIANTS
   ============================================================================= */

/* Primary (Blue) - View/Open actions */
.resource-button-group .btn-primary,
.resource-button-group button[data-action="view"],
.resource-button-group a[href*="viewer"] {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
}

.resource-button-group .btn-primary:hover,
.resource-button-group button[data-action="view"]:hover,
.resource-button-group a[href*="viewer"]:hover {
    background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
    box-shadow: 0 4px 12px rgba(59, 130, 246, 0.35);
}

/* Download (Purple) */
.resource-button-group .btn-download,
.resource-button-group button[data-action="download"] {
    background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
}

.resource-button-group .btn-download:hover,
.resource-button-group button[data-action="download"]:hover {
    background: linear-gradient(135deg, #7c3aed 0%, #6d28d9 100%);
    box-shadow: 0 4px 12px rgba(139, 92, 246, 0.35);
}

/* Send (Cyan) */
.resource-button-group .btn-send,
.resource-button-group button[data-action="send"] {
    background: linear-gradient(135deg, #06b6d4 0%, #0891b2 100%);
}

.resource-button-group .btn-send:hover,
.resource-button-group button[data-action="send"]:hover {
    background: linear-gradient(135deg, #0891b2 0%, #0e7490 100%);
    box-shadow: 0 4px 12px rgba(6, 182, 212, 0.35);
}

/* Edit (Orange) */
.resource-button-group .btn-edit,
.resource-button-group button[data-action="edit"] {
    background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
}

.resource-button-group .btn-edit:hover,
.resource-button-group button[data-action="edit"]:hover {
    background: linear-gradient(135deg, #d97706 0%, #b45309 100%);
    box-shadow: 0 4px 12px rgba(245, 158, 11, 0.35);
}

/* Delete (Red) */
.resource-button-group .btn-danger,
.resource-button-group .btn-delete,
.resource-button-group button[data-action="delete"] {
    background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
}

.resource-button-group .btn-danger:hover,
.resource-button-group .btn-delete:hover,
.resource-button-group button[data-action="delete"]:hover {
    background: linear-gradient(135deg, #dc2626 0%, #b91c1c 100%);
    box-shadow: 0 4px 12px rgba(239, 68, 68, 0.35);
}

/* Success (Green) */
.resource-button-group .btn-success,
.resource-button-group button[data-action="approve"] {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
}

.resource-button-group .btn-success:hover,
.resource-button-group button[data-action="approve"]:hover {
    background: linear-gradient(135deg, #059669 0%, #047857 100%);
    box-shadow: 0 4px 12px rgba(16, 185, 129, 0.35);
}

/* =============================================================================
   BUTTON STATES
   ============================================================================= */

/* Disabled */
.resource-button-group button:disabled,
.resource-button-group .btn:disabled {
    background: linear-gradient(135deg, #cbd5e1 0%, #94a3b8 100%);
    color: #64748b;
    cursor: not-allowed;
    box-shadow: none;
    opacity: 0.5;
}

.resource-button-group button:disabled:hover,
.resource-button-group .btn:disabled:hover {
    transform: none;
    box-shadow: none;
}

/* Loading */
.resource-button-group button.loading,
.resource-button-group .btn.loading {
    color: transparent;
    pointer-events: none;
}

.resource-button-group button.loading::after,
.resource-button-group .btn.loading::after {
    content: '';
    position: absolute;
    width: 20px;
    height: 20px;
    border: 2px solid rgba(255, 255, 255, 0.3);
    border-top-color: white;
    border-radius: 50%;
    animation: spin 0.6s linear infinite;
}

@keyframes spin {
    to {
        transform: rotate(360deg);
    }
}

/* Active/Selected */
.resource-button-group button.active,
.resource-button-group .btn.active {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
    box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
    transform: scale(1.05);
}

/* =============================================================================
   TOOLTIPS
   ============================================================================= */

.resource-button-group button[title],
.resource-button-group .btn[title] {
    position: relative;
}

/* Показывать tooltip при hover */
.resource-button-group button[title]:hover::before,
.resource-button-group .btn[title]:hover::before {
    content: attr(title);
    position: absolute;
    right: calc(100% + 8px);
    top: 50%;
    transform: translateY(-50%);
    background: rgba(0, 0, 0, 0.9);
    color: white;
    padding: 6px 12px;
    border-radius: 6px;
    font-size: 12px;
    font-weight: 500;
    white-space: nowrap;
    pointer-events: none;
    z-index: 1000;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
}

/* Стрелка tooltip */
.resource-button-group button[title]:hover::after,
.resource-button-group .btn[title]:hover::after {
    content: '';
    position: absolute;
    right: calc(100% + 2px);
    top: 50%;
    transform: translateY(-50%);
    border: 6px solid transparent;
    border-left-color: rgba(0, 0, 0, 0.9);
    pointer-events: none;
    z-index: 1000;
}

/* =============================================================================
   SMALL BUTTONS (для компактного отображения)
   ============================================================================= */

.resource-button-group.small button,
.resource-button-group.small .btn,
.resource-button-group .btn-sm {
    width: 40px;
    height: 40px;
    min-width: 40px;
    font-size: 16px;
    border-radius: 8px;
}

.resource-button-group.small button i,
.resource-button-group .btn-sm i {
    font-size: 16px;
}

/* =============================================================================
   LARGE BUTTONS
   ============================================================================= */

.resource-button-group.large button,
.resource-button-group.large .btn,
.resource-button-group .btn-lg {
    width: 56px;
    height: 56px;
    min-width: 56px;
    font-size: 24px;
    border-radius: 12px;
}

.resource-button-group.large button i,
.resource-button-group .btn-lg i {
    font-size: 24px;
}

/* =============================================================================
   DROPDOWN BUTTONS
   ============================================================================= */

.resource-button-group .dropdown-toggle {
    position: relative;
}

.resource-button-group .dropdown-toggle::after {
    content: '';
    position: absolute;
    right: 4px;
    bottom: 4px;
    width: 0;
    height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 4px solid white;
}

/* =============================================================================
   RESPONSIVE DESIGN
   ============================================================================= */

@media (max-width: 768px) {
    .resource-button-group {
        flex-direction: row;
        justify-content: center;
        gap: 8px;
    }

    .resource-button-group button,
    .resource-button-group .btn {
        width: 44px;
        height: 44px;
        min-width: 44px;
        font-size: 18px;
    }
}

/* =============================================================================
   ACCESSIBILITY
   ============================================================================= */

.resource-button-group button:focus-visible,
.resource-button-group .btn:focus-visible {
    outline: 2px solid #3b82f6;
    outline-offset: 2px;
}

/* High contrast mode */
@media (prefers-contrast: high) {

    .resource-button-group button,
    .resource-button-group .btn {
        border: 2px solid rgba(255, 255, 255, 0.3);
    }
}

/* Reduced motion */
@media (prefers-reduced-motion: reduce) {

    .resource-button-group button,
    .resource-button-group .btn {
        transition: none;
        animation: none;
    }
}

/* =============================================================================
   PRINT STYLES
   ============================================================================= */

@media print {
    .resource-button-group {
        display: none !important;
    }
}
</style>