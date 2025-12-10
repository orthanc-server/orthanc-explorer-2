<script>
import Modal from "./Modal.vue"
import ShareModal from "./ShareModal.vue"
import ModifyModal from "./ModifyModal.vue"
import $ from "jquery"
import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: ["resourcesOrthancId", "linkUrl", "level", "tokenType", "validityDuration", "title", "iconClass", "opensInNewTab", "linkType", "disabled", "smallIcons"],
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
            if (this.tokenType == "download-instant-link" && this.advancedOptions.DownloadInstantLinksUseRestApiWithAuthHeaders) {
                api.downloadFileWithAuthHeaders(this.linkUrl);
                event.preventDefault();
                return;
            }

            if (!this.tokens.RequiredForLinks && !this.advancedOptions.InstantLinksReuseTokenFromUri) {
                return;  // just execute the default click handler: no token in url, no HTTP headers
            }
            event.preventDefault();

            let token = null;
            if (!this.tokens.RequiredForLinks && this.advancedOptions.InstantLinksReuseTokenFromUri) {
                token = {
                    "Token": localStorage.getItem(this.advancedOptions.InstantLinksReuseTokenFromUri)
                };
                console.log("reusing token from uri: ", this.advancedOptions.InstantLinksReuseTokenFromUri, token["Token"]);
            } else {
                // generate a token through the auth-plugin & auth-service
                let validityDuration = this.validityDuration;
                if (validityDuration == null || validityDuration === undefined) {
                    validityDuration = this.tokens.InstantLinksValidity;
                }
                let level = this.level;
                if (level == "bulk-study") {
                    level = "study";
                }

                token = await api.createToken({ tokenType: this.tokenType, resourcesIds: this.resourcesOrthancId, level: level, validityDuration: validityDuration });
            }

            let finalUrl = this.linkUrl;

            // give priority to the urls coming from the token service
            if (token["Url"] != null) {
                finalUrl = token["Url"];
            }
            else if (finalUrl.startsWith("weasis:")) {
                const extraParts = ["-H", `"Authorization: Bearer ${token["Token"]}"`];
                finalUrl = finalUrl + "+" + extraParts.map(v => encodeURIComponent(v)).join("+");
            } else if (finalUrl.indexOf('?') == -1) {
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
            advancedOptions: state => state.configuration.advancedOptions,
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
        isIcon() {
            return this.linkType == "icon";
        },
        classes() {
            let c = ["btn-secondary"];

            if (this.disabled) {
                c.push("disabled-link");
            }

            if (this.smallIcons) {
                c.push("btn-icon-small");
            } else {
                c.push("btn-icon");
            }

            return c.join(" ");
        },
        divClasses() {
            if (this.linkType == "icon") {
                return "div-icon"
            } else {
                return "";
            }
        }
    },
    components: {}
}
</script>

<template>
    <div :class="divClasses">
        <a v-if="isButton" class="btn btn-sm m-1" type="button"
            data-bs-toggle="tooltip" :title="title" @click="clicked" :target="target" :href="linkUrl" :class="classes">
            <i :class="iconClass"></i>
        </a>
        <a v-if="isDropDownItem" class="dropdown-item" :target="target" @click="clicked"
            :href="linkUrl">{{ title }}
        </a>
        <a v-if="isIcon" class="icon" :target="target" @click="clicked"
            data-bs-toggle="tooltip" :title="title" :href="linkUrl">
            <i :class="iconClass" style="padding-top: 4px;"></i>
        </a>
    </div>
</template>

<style>
/* this mimicks the disabled state of btn-secondary */
.disabled-link {
    pointer-events: none;
    background-color: #6c757d;
    opacity: 0.65;
}

.icon {
    color: var(--bs-table-color);
    width: 100%;    /* increase the size of the clickable area */
    height: 100%;
    display: flex;
    justify-content: center;
    align-items: center;    
}

.div-icon {
    width: 100%;    /* increase the size of the clickable area */
    height: 100%;
    display: flex;
    justify-content: center;
    align-items: center;    
}
</style>