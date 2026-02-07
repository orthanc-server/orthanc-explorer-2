<script>
import { mapState } from "vuex"
import api from "../orthancApi";
import Tags from "bootstrap5-tags/tags.js"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import LabelsEditor from "./LabelsEditor.vue";


export default {
    props: ["resourcesOrthancId", "resourceLevel"],
    emits: ["bulkModalClosed"],
    data() {
        return {
            labelsToRemove: [],
            labelsToAdd: [],
            removedLabels: [],
            messages: [],
            addInProgress: false,
            clearAllInProgress: false,
            removeInProgress: false
        }
    },
    async mounted() {
        this.reset();
        // Tags.init();

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
        });
        this.$refs['modal-main-div'].addEventListener('hide.bs.modal', (e) => {
            this.$emit("bulkModalClosed");
        });
    },
    methods: {
        async showModal() {
            var myModal = new bootstrap.Modal(this.$el, {});
            myModal.show();
        },
        async reset() {
            this.labelsToRemove = [];
            this.labelsToAdd = [];
            this.removedLabels = [];
            this.messages = [];
            this.addInProgress = false;
            this.clearAllInProgress = false;
            this.removeInProgress = false;
        },
        async clearAllLabels() {
            this.clearAllInProgress = true;
            let promises = [];

            // ✅ Создаём все промисы сразу (параллельно)
            for (const studyId of this.resourcesOrthancId) {
                promises.push(api.removeAllLabels(studyId)); // Убрали await
            }

            const promisesResults = await Promise.all(promises);
            let removedLabels = new Set();
            for (const result of promisesResults) {
                for (const label of result) {
                    removedLabels.add(label);
                }
            }
            this.messages.push({
                labels: removedLabels,
                part1: this.$t('labels.cleared_labels_message_part_1_html', { count: this.resourcesOrthancId.length }),
                part2: this.$t('labels.cleared_labels_message_part_2_html', { count: this.resourcesOrthancId.length })
            });
            this.clearAllInProgress = false;

            this.$store.dispatch('studies/refreshStudiesLabels', { studiesIds: this.resourcesOrthancId });
        },
        onLabelsToAddChanged(labelsToAdd) {
            this.labelsToAdd = labelsToAdd;
        },
        onLabelsToRemoveChanged(labelsToRemove) {
            this.labelsToRemove = labelsToRemove;
        },
        async addLabels() {
            this.addInProgress = true;
            let promises = [];

            for (const label of this.labelsToAdd) {
                for (const studyId of this.resourcesOrthancId) {
                    promises.push(api.addLabel({
                        studyId: studyId,
                        label: label
                    }))
                }
            }

            const promisesResults = await Promise.all(promises);
            let processedLabels = new Set();
            for (const label of promisesResults) {
                processedLabels.add(label);
            }
            this.messages.push({
                labels: processedLabels,
                part1: this.$t('labels.added_labels_message_part_1_html', { count: this.resourcesOrthancId.length }),
                part2: this.$t('labels.added_labels_message_part_2_html', { count: this.resourcesOrthancId.length })
            });
            this.addInProgress = false;

            this.$store.dispatch('studies/refreshStudiesLabels', { studiesIds: this.resourcesOrthancId });
        },
        isLabelToAdd(label) {
            return this.labelsToAdd.includes(label);
        },
        isLabelToRemove(label) {
            return this.labelsToRemove.includes(label);
        },
        async removeLabels() {
            this.removeInProgress = true;
            let promises = [];

            for (const label of this.labelsToRemove) {
                for (const studyId of this.resourcesOrthancId) {
                    promises.push(api.removeLabel({
                        studyId: studyId,
                        label: label
                    }));
                }
            }
            const promisesResults = await Promise.all(promises);
            let processedLabels = new Set();
            for (const label of promisesResults) {
                processedLabels.add(label);
            }
            this.messages.push({
                labels: processedLabels,
                part1: this.$t('labels.removed_labels_message_part_1_html', { count: this.resourcesOrthancId.length }),
                part2: this.$t('labels.removed_labels_message_part_2_html', { count: this.resourcesOrthancId.length })
            });
            this.removeInProgress = false;

            this.$store.dispatch('studies/refreshStudiesLabels', { studiesIds: this.resourcesOrthancId });
        },
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            allLabels: state => state.labels.allLabels
        }),
        isAddButtonEnabled() {
            return !this.addInProgress && this.labelsToAdd.length > 0;
        },
        isRemoveButtonEnabled() {
            return !this.removeInProgress && this.labelsToRemove.length > 0;
        }
    },
    components: { LabelsEditor },
};
</script>

<template>
    <div class="modal fade" id="bulk-labels-modal" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <div class="modal-dialog modal-lg modal-dialog-scrollable">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title" id="modalLabel">{{ $t("labels.modal_title") }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>

                <div class="modal-body">
                    <div class="container">
                        <div class="row border-bottom py-3">
                            <div class="col-md-9">
                                <p v-html="$t('labels.clear_all_labels_message_html')"></p>
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100" @click="clearAllLabels()"
                                    :disabled="clearAllInProgress">
                                    <span>{{ $t('labels.clear_all_button') }}</span>
                                    <span v-if="clearAllInProgress" class="spinner-border spinner-border-sm alert-icon"
                                        role="status" aria-hidden="true"></span>
                                </button>
                            </div>
                        </div>
                        <div class="row border-bottom py-3">
                            <div class="col-md-9">
                                <LabelsEditor id="addLabels" :labels="labelsToAdd"
                                    :title="'labels.add_labels_message_html'" :studyId="null"
                                    @labelsUpdated="onLabelsToAddChanged"></LabelsEditor>
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100" @click="addLabels()"
                                    :disabled="!isAddButtonEnabled">
                                    <span>{{ $t('labels.add_button') }}</span>
                                    <span v-if="addInProgress" class="spinner-border spinner-border-sm alert-icon"
                                        role="status" aria-hidden="true"></span>
                                </button>
                            </div>
                        </div>
                        <div class="row border-bottom py-3">
                            <div class="col-md-9">
                                <LabelsEditor id="removeLabels" :labels="labelsToRemove"
                                    :title="'labels.remove_labels_message_html'" :studyId="null"
                                    @labelsUpdated="onLabelsToRemoveChanged"></LabelsEditor>
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100" @click="removeLabels()"
                                    :disabled="!isRemoveButtonEnabled">
                                    <span>{{ $t('labels.remove_button') }}</span>
                                    <span v-if="removeInProgress" class="spinner-border spinner-border-sm alert-icon"
                                        role="status" aria-hidden="true"></span>
                                </button>
                            </div>
                        </div>
                        <div class="row border-bottom py-3">
                            <div class="col-md-12">
                                <p v-for="message in messages" :key="message">
                                    <span v-html="message.part1"></span>
                                    <span v-for="label in message.labels" :key="label" class="label badge">{{ label
                                        }}</span>
                                    <span v-html="message.part2"></span>
                                </p>
                            </div>
                        </div>
                        <div class="row py-3">
                            <div class="col-md-12">
                                <button type="button" class="btn btn-primary w-100" data-bs-dismiss="modal"
                                    aria-label="Close">{{ $t('close') }}</button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<style></style>