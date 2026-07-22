<script>
import InstanceDetails from "./InstanceDetails.vue";
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import SelectionStatus from "../helpers/selection-status";
import SourceType from '../helpers/source-type';
import { mapState, mapGetters } from "vuex"


export default {
    props: ["instanceId", "instanceInfo", 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags', 'studyId', 'seriesId', 'studySeries', 'seriesInstances'],
    emits: ['deletedInstance'],
    data() {
        return {
            loaded: false,
            expanded: false,
            collapseElement: null
        };
    },
    mounted() {
        this.$refs['instance-collapsible-details'].addEventListener('show.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = true;
            }
        });
        this.$refs['instance-collapsible-details'].addEventListener('hide.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = false;
            }
        });

        var el = this.$refs['instance-collapsible-details'];
        this.collapseElement = new bootstrap.Collapse(el, { toggle: false });

        for (const [k, v] of Object.entries(this.$route.query)) {
            if (k === 'expand') {
                if (v === 'instance') {
                    this.collapseElement.show();
                }
            }
        }
    },
    computed: {
        ...mapState({
            studiesSourceType: state => state.studies.sourceType,
        }),
        isMultipleSelectionEnabled() {
            return this.studiesSourceType != SourceType.REMOTE_DICOM;
        },
        isSelected() {
            return this.$store.getters['selection/getInstanceSelectionStatus'](this.studyId, this.seriesId, this.instanceId) != SelectionStatus.NOT_SELECTED;
        },
        isPartialySelected() {
            return this.$store.getters['selection/getInstanceSelectionStatus'](this.studyId, this.seriesId, this.instanceId) == SelectionStatus.PARTIAL;
        }
    },
    methods: {
        onDeletedInstance(instanceId) {
            this.$emit('deletedInstance', instanceId);
        },
        async clickedSelect() {
            await this.$store.dispatch('selection/selectInstance', { studyId: this.studyId, seriesId: this.seriesId, studySeries: this.studySeries, instanceId: this.instanceId, seriesInstances: this.seriesInstances, isSelected: !this.isSelected }); // this.selected is the value before the click
        }
    },
    components: { InstanceDetails }
}
</script>


<template>
    <tbody>
        <tr :class="{ 'instance-row-collapsed': !expanded, 'instance-row-expanded': expanded }">
            <td>
                <div v-if="isMultipleSelectionEnabled" class="form-check">
                    <input class="form-check-input" type="checkbox" :checked="isSelected" :indeterminate="isPartialySelected" @click="clickedSelect">
                </div>
            </td>
            <td class="cut-text text-center" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#instance-details-' + this.instanceId">{{
                    instanceInfo.MainDicomTags.InstanceNumber }}</td>
            <td class="cut-text" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#instance-details-' + this.instanceId">{{
                    instanceInfo.MainDicomTags.SOPInstanceUID }}</td>
            <td class="cut-text text-center" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#instance-details-' + this.instanceId">{{
                    instanceInfo.MainDicomTags.NumberOfFrames }}</td>
        </tr>
        <tr class="collapse" :class="{ 'instance-details-collapsed': !expanded, 'instance-details-expanded': expanded }"
            v-bind:id="'instance-details-' + this.instanceId" ref="instance-collapsible-details">
            <td colspan="100">
                <InstanceDetails v-if="this.expanded" :instanceId="this.instanceId"
                    :studyMainDicomTags="this.studyMainDicomTags" :seriesMainDicomTags="this.seriesMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags" @deletedInstance="onDeletedInstance">
                </InstanceDetails>
            </td>
        </tr>
    </tbody>
</template>

<style scoped>
.instance-row-collapsed {}

.instance-row-expanded {
    background-color: var(--instance-details-bg-color);
    font-weight: 700;

    border-top: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.instance-row-expanded>:first-child {
    border-bottom: 0px !important;
}


.instance-details-expanded {
    background-color: var(--instance-details-bg-color);

    border-top: 0px !important;
    border-bottom: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}
</style>