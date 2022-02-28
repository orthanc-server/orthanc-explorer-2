<script>
import InstanceDetails from "./InstanceDetails.vue";

export default {
    props: ["instanceId", "instanceInfo"],
    emits: ['deletedInstance'],
    data() {
        return {
            loaded: false,
            expanded: false,
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
    },
    methods: {
        onDeletedInstance(instanceId) {
            this.$emit('deletedInstance', instanceId);
        }
    },
    computed: {
    },
    components: { InstanceDetails }
}
</script>


<template>
    <tbody>
        <tr :class="{ 'instance-row-collapsed': !expanded, 'instance-row-expanded': expanded }">
            <td></td>
            <td
                class="cut-text text-center"
                data-bs-toggle="collapse"
                v-bind:data-bs-target="'#instance-details-' + this.instanceId"
            >{{ instanceInfo.MainDicomTags.InstanceNumber }}</td>
            <td
                class="cut-text"
                data-bs-toggle="collapse"
                v-bind:data-bs-target="'#instance-details-' + this.instanceId"
            >{{ instanceInfo.MainDicomTags.SOPInstanceUID }}</td>
            <td
                class="cut-text text-center"
                data-bs-toggle="collapse"
                v-bind:data-bs-target="'#instance-details-' + this.instanceId"
            >{{ instanceInfo.MainDicomTags.NumberOfFrames }}</td>
        </tr>
        <tr
            class="collapse"
            :class="{ 'instance-details-collapsed': !expanded, 'instance-details-expanded': expanded }"
            v-bind:id="'instance-details-' + this.instanceId"
            ref="instance-collapsible-details"
        >
            <td colspan="100">
                <InstanceDetails
                    v-if="this.expanded"
                    :instanceId="this.instanceId"
                    @deletedInstance="onDeletedInstance"
                ></InstanceDetails>
            </td>
        </tr>
    </tbody>
</template>

<style scoped>

.instance-row-collapsed {
}

.instance-row-expanded {
    background-color: var(--instance-selected-color);
    font-weight: 700;

    border-top: 2px !important;
    border-style: solid !important;
    border-color: black !important;    
}

.instance-row-expanded>:first-child {
    border-bottom: 0px !important;
}

.instance-table > tbody > tr:hover {
    background-color: var(--instance-hover-color);
}

.instance-table > tbody > tr.instance-row-expanded:hover {
    background-color: var(--instance-selected-color);
}
.instance-table > tbody > tr.instance-details-expanded:hover {
    background-color: var(--instance-selected-color);
}


.instance-details-expanded {
    background-color: var(--instance-selected-color);

    border-top: 0px !important;
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}
</style>