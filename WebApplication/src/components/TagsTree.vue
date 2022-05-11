<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";

export default {
    props: ['tags'],
    data() {
        return {
        }
    },
    mounted() {
    },
    computed: {

    },
    methods: {
    },
    components : { CopyToClipboardButton }

}
</script>

<template>
    <ul>
        <li v-for="(item, key) in tags" :key="key">
            <span v-if="item.Type=='Sequence'">
                <span class="details-label">{{key}} - {{item.Name}}: </span>
                    <ul>
                        <li v-for="(subItem, subIndex) in item.Value" :key="subItem">
                            Item {{subIndex + 1}}
                        <tags-tree
                            :tags="subItem">
                        </tags-tree>
                        </li>
                    </ul>
            </span>
            <span v-else class="d-flex w-100">
                <span class="details-label">{{key}} - {{item.Name}}: </span>
                <span v-if="item.Type=='String'" class="details">{{item.Value}}</span>
                <CopyToClipboardButton :valueToCopy="item.Value" />
            </span>
        </li>
    </ul>
</template>

<style scoped>

.details-label {
    font-weight: 700;
    max-width: 30%;
    /* overflow: hidden;
    text-overflow: ellipsis; */
}

.details {
    margin-left: auto !important;
    font-weight: 500;
    /* overflow: hidden;
    text-overflow: ellipsis; */
}

ul {
    list-style: none;
    border-bottom-width: 1px;
    border-bottom-style: solid;
    border-bottom-color: #80808080;
}

ul li {
    display: block;
    border-bottom-width: 1px;
    border-bottom-style: solid;
    border-bottom-color: #80808080;
}

ul:last-child {
    border:none;
}

ul li:last-child {
    border:none;
}

</style>

