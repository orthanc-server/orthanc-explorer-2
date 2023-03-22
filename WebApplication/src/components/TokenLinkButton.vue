<script>
import Modal from "./Modal.vue"
import ShareModal from "./ShareModal.vue"
import ModifyModal from "./ModifyModal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"


export default {
    props: ["resourcesOrthancId", "linkUrl", "level", "tokenType", "validityDuration", "title", "iconClass", "opensInNewTab", "linkType", "disabled"],
    setup() {
        return {
        }
    },

    data() {
        return {
        };
    },
    mounted() {
    },
    methods: {
        async clicked(event) {
            if (!this.tokens.RequiredForLinks) {
                return;  // just execute the default click handler
            }
            event.preventDefault();
            let validityDuration = this.validityDuration;
            if (validityDuration == null || validityDuration === undefined) {
                validityDuration = this.tokens.InstantLinksValidity;
            }
            let token = await api.createToken({ tokenType: this.tokenType, resourcesIds: this.resourcesOrthancId, level: this.level, validityDuration: validityDuration });
            let finalUrl = this.linkUrl;

            // give priority to the urls coming from the token service
            if (token["Url"] != null) {
                finalUrl = token["Url"];
            }
            else if (this.linkUrl.indexOf('?') == -1) {
                finalUrl = finalUrl + "?token=" + token["Token"];
            } else {
                finalUrl = finalUrl + "&token=" + token["Token"];
            }

            let link = document.querySelector("#hidden-link-for-tokens");
            if (link == null) {
                link = document.createElement('a');
                document.body.appendChild(link);
                link.setAttribute("id", "hidden-link-for-tokens");
                link.setAttribute("type", "hidden");
            }
            link.target = this.target;
            link.href = finalUrl;
            link.click();
        },
    },
    computed: {
        ...mapState({
            tokens: state => state.configuration.tokens,
        }),
        target() {
            if (this.opensInNewTab === undefined || this.opensInNewTab == false) {
                return "_self";
            } else {
                return "_blank";
            }
        },
        isButton() {
            return this.linkType === undefined || this.linkType == "button";
        },
        isDropDownItem() {
            return this.linkType == "dropdown-item";
        },
        classes() {
            if (this.disabled) {
                return "btn-secondary disabled-link";
            } else {
                return "btn-secondary";
            }
        }
    },
    components: {}
}
</script>

<template>
    <div>
        <a v-if="isButton" class="btn btn-sm m-1" type="button"
            data-bs-toggle="tooltip" :title="title" @click="clicked" :target="target" :href="linkUrl" :class="classes">
            <i :class="iconClass"></i>
        </a>
        <a v-if="isDropDownItem" class="dropdown-item" :target="target" @click="clicked"
            :href="linkUrl">{{ title }}</a>
    </div>
</template>

<style>
/* this mimicks the disabled state of btn-secondary */
.disabled-link {
    pointer-events: none;
    background-color: #6c757d;
    opacity: 0.65;
}
</style>