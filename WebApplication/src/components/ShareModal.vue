<script>

export default {
  props: ["id", "studyTags"],
  methods: {
    share() {
    }
  },
  mounted() {
    this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
      // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
      document.querySelector('body').appendChild(e.target);
    });
  }
};
</script>

<template>
  <div
    class="modal fade"
    :id="this.id"
    tabindex="-1"
    aria-labelledby="modalLabel"
    ref="modal-main-div"
  >
    <!-- aria-hidden="true" -->
    <div class="modal-dialog">
      <div class="modal-content">
        <div class="modal-header">
          <h5 class="modal-title" id="modalLabel">Share study  </h5>
          <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
        </div>
        <div class="modal-body">
            body
        </div>
        <div class="modal-footer">
          <button
            v-if="cancelText"
            type="button"
            class="btn btn-secondary"
            data-bs-dismiss="modal"
          >Cancel</button>
          <button
            type="button"
            class="btn btn-primary"
            data-bs-dismiss="modal"
            @click="share()"
          >Share</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style>
</style>