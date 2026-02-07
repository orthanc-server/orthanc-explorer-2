<script>
import SeriesDetails from "./SeriesDetails.vue";
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import { mapState, mapGetters } from "vuex"
import SourceType from '../helpers/source-type';


export default {
    props: ["seriesId", "seriesInfo", 'studyMainDicomTags', 'patientMainDicomTags'],
    emits: ['deletedSeries'],
    data() {
        return {
            loaded: false,
            expanded: false,
            collapseElement: null
        };
    },
    computed: {
        ...mapState({
            studiesSourceType: state => state.studies.sourceType,
        }),
        instancesCount() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
                return this.seriesInfo.Instances.length;
            } else if (this.studiesSourceType == SourceType.REMOTE_DICOM || this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
                return this.seriesInfo.MainDicomTags.NumberOfSeriesRelatedInstances;
            }
        }
    },
    mounted() {
        this.$refs['series-collapsible-details'].addEventListener('show.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = true;
            }
        });
        this.$refs['series-collapsible-details'].addEventListener('hide.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = false;
            }
        });

        var el = this.$refs['series-collapsible-details'];
        this.collapseElement = new bootstrap.Collapse(el, { toggle: false });

        for (const [k, v] of Object.entries(this.$route.query)) {
            if (k === 'expand') {
                if (v === 'series' || v === 'instance') {
                    this.collapseElement.show();
                }
            }
        }
    },
    methods: {
        onDeletedSeries() {
            this.$emit("deletedSeries", this.seriesId);
        }
    },
    components: { SeriesDetails }
}
</script>


<template>
    <tbody>
        <tr :class="{ 'series-row-collapsed': !expanded, 'series-row-expanded': expanded }">
            <td></td>
            <td class="cut-text text-center" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#series-details-' + this.seriesId">{{ seriesInfo.MainDicomTags.SeriesNumber }}
            </td>
            <td class="cut-text" data-bs-toggle="collapse" v-bind:data-bs-target="'#series-details-' + this.seriesId">
                <span data-bs-toggle="tooltip" v-bind:title="seriesInfo.MainDicomTags.SeriesDescription">{{
                    seriesInfo.MainDicomTags.SeriesDescription }}</span>
            </td>
            <td class="cut-text text-center" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#series-details-' + this.seriesId">{{ seriesInfo.MainDicomTags.Modality }}</td>
            <td class="cut-text text-center" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#series-details-' + this.seriesId">{{ instancesCount }}</td>
        </tr>
        <tr class="collapse" :class="{ 'series-details-collapsed': !expanded, 'series-details-expanded': expanded }"
            v-bind:id="'series-details-' + this.seriesId" ref="series-collapsible-details">
            <td colspan="100">
                <SeriesDetails v-if="this.expanded" :seriesId="this.seriesId" :instancesIds="this.seriesInfo.Instances"
                    :seriesMainDicomTags="this.seriesInfo.MainDicomTags" :studyMainDicomTags="this.studyMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags" @deletedSeries="onDeletedSeries"></SeriesDetails>
            </td>
        </tr>
    </tbody>
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