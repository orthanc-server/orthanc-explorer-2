<script setup>
// This starter template is using Vue 3 <script setup> SFCs
// Check out https://v3.vuejs.org/api/sfc-script-setup.html#sfc-script-setup
import StudyList from "./components/StudyList.vue";
import UploadHandler from "./components/UploadHandler.vue"
import Modal from "./components/Modal.vue";
import SideBar from "./components/SideBar.vue";
import api from "./orthancApi"


var tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'))
var tooltipList = tooltipTriggerList.map(function (tooltipTriggerEl) {
    return new bootstrap.Tooltip(tooltipTriggerEl)
})
</script>

<script>
export default {
    async created() {
        console.log("Creating App...");
        await this.$store.dispatch('configuration/load');
        await this.$store.dispatch('studies/initialLoad');
        console.log("App created");
    },

}
</script>

<template>
    <div class="full-page">
        <div class="nav-side-menu">
            <router-view name="SideBarView"></router-view>
        </div>
        <div class="content">
            <router-view name="ContentView"></router-view>
        </div>
    </div>
</template>

<style>
#app {
    font-family: Avenir, Helvetica, Arial, sans-serif;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    text-align: center;
    color: #2c3e50;
    margin-top: 10px;
}
</style>