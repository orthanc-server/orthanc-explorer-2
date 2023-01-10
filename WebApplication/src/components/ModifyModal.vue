<script>
import { mapState } from "vuex"
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import api from "../orthancApi"


export default {
    props: ["orthancId", "studyMainDicomTags", "patientMainDicomTags"],
    methods: {
        modify() {
        }
    },
    data() {
        return {
            tags: {},
            createNewStudy: true,
            showCreateNewStudyOption: true
        }
    },
    mounted() {
        console.log("ModifyModal mounted");
        this.tags = {};
        for (const [k, v] of Object.entries(this.patientMainDicomTags)) {
            this.tags[k] = v
        }
        for (const [k, v] of Object.entries(this.studyMainDicomTags)) {
            if (k != "StudyInstanceUID") {
                this.tags[k] = v
            }
        }

        this.createNewStudy = ["Always", "DefaultYes"].indexOf(this.uiOptions.ModifyCreatesNewStudy) != -1;
        this.showCreateNewStudyOption = ["DefaultYes", "DefaultNo"].indexOf(this.uiOptions.ModifyCreatesNewStudy) != -1;

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
        });
    },
    watch: {
    },
    methods: {
        async modify() {
            // const anonymized = this.anonymized == "yes";
            const r = await api.modifyStudy(this.orthancId, {"PatientID": "TOTO"}, []);
            console.log("modified ", r);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        resourceTitle() {
            return resourceHelpers.getResourceTitle("study", this.patientMainDicomTags, this.studyMainDicomTags, null, null);
        },
        isModified() {
            return true;
        }
    },
    components: { CopyToClipboardButton }

};
</script>

<template>
    <div class="modal fade" :id="this.orthancId" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title" id="modalLabel">{{ $t("modify.modal_title") + " " + resourceTitle }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="container">
                        <div v-for="(item, key) in tags" :key="key" class="row">
                            <div class="col-md-5">
                                {{ key }}
                            </div>
                            <div class="col-md-6">
                                {{ item }}
                            </div>
                            <div class="col-md-1">
                                <div class="d-flex flex-row-reverse">
                                    <CopyToClipboardButton :valueToCopy="item" />
                                </div>
                            </div>
                        </div>
                        </div>
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary"
                        data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button v-if="isModified" type="button" class="btn btn-primary"
                        @click="modify()">{{ $t("modify.button_title") }}</button>
                </div>
            </div>
        </div>
    </div>
</template>

<style>
/* The switch - the box around the slider */
.switch {
  position: relative;
  display: inline-block;
  width: 60px;
  height: 34px;
}

/* Hide default HTML checkbox */
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

/* The slider */
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  -webkit-transition: .4s;
  transition: .4s;
}

.slider:before {
  position: absolute;
  content: "";
  height: 26px;
  width: 26px;
  left: 4px;
  bottom: 4px;
  background-color: white;
  -webkit-transition: .4s;
  transition: .4s;
}

input:checked + .slider {
  background-color: #2196F3;
}

input:focus + .slider {
  box-shadow: 0 0 1px #2196F3;
}

input:checked + .slider:before {
  -webkit-transform: translateX(26px);
  -ms-transform: translateX(26px);
  transform: translateX(26px);
}

/* Rounded sliders */
.slider.round {
  border-radius: 34px;
}

.slider.round:before {
  border-radius: 50%;
}
</style>