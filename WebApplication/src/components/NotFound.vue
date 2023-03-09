<script>
export default {
    props: [],
    data() {
        return {
            path: null
        };
    },
    mounted() {
        this.path = this.$router.currentRoute.value.fullPath;
    }
}

</script>
<template>
    <div class="alert alert-danger" role="alert">
        {{$t('page_not_found')}} <br/>
        {{path}}
    </div>
</template>
<style scoped>
</style>