<script>
function applyBootStrapTheme() {
  // hack to switch the theme: get the value from our custom css
  let bootstrapTheme = getComputedStyle(
    document.documentElement,
  ).getPropertyValue("--bootstrap-theme");
  console.log("-------------- Applying Bootstrap theme ...", bootstrapTheme);
  if (bootstrapTheme) {
    // and set it to the 'html' element
    document.documentElement.setAttribute("data-bs-theme", bootstrapTheme);
  } else {
    console.warn(
      "-------------- Applying Bootstrap theme not defined yet, retrying ...",
    );
    setTimeout(applyBootStrapTheme, 100);
  }
}

export default {
  data() {
    return {
      sidebarCollapsed: false,
    };
  },
  async created() {
    console.log("Creating App...");

    {
      // Load the CSS dynamically since it can be a custom css
      console.log("Loading the defaults + custom CSS ...");

      let link = document.createElement("link");
      link.rel = "stylesheet";
      link.type = "text/css";
      link.href = "customizable/custom.css";

      document.getElementsByTagName("HEAD")[0].appendChild(link);

      setTimeout(applyBootStrapTheme, 0);
    }

    await this.$store.dispatch("configuration/load");
    await this.$store.dispatch("studies/initialLoad");
    await this.$store.dispatch("labels/refresh");
    console.log("App created");

    // Restore sidebar state from localStorage
    const savedState = localStorage.getItem("sidebarCollapsed");
    if (savedState !== null) {
      this.sidebarCollapsed = savedState === "true";
    }

    // Listen for sidebar toggle events
    window.addEventListener("sidebar-collapsed", this.handleSidebarToggle);
  },
  beforeUnmount() {
    window.removeEventListener("sidebar-collapsed", this.handleSidebarToggle);
  },
  methods: {
    handleSidebarToggle(e) {
      this.sidebarCollapsed = e.detail.collapsed;
    },
  },
};
</script>

<template>
  <div class="full-page" :class="{ 'sidebar-collapsed': sidebarCollapsed }">
    <div class="nav-side-layout" :class="{ collapsed: sidebarCollapsed }">
      <router-view name="SideBarView"></router-view>
    </div>
    <div class="content" :class="{ 'sidebar-collapsed': sidebarCollapsed }">
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
  width: 99.9%;
}

/* ============================================
   SIDEBAR LAYOUT
   ============================================ */
.nav-side-layout {
  overflow: visible !important;
  position: fixed;
  top: 0;
  left: 0;
  width: 220px;
  height: 100%;
  background-color: var(--nav-side-bg-color);
  color: var(--nav-side-color);
  z-index: 1000;
  transition: width 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.nav-side-layout.collapsed {
  width: 60px;
}

.nav-side-layout .toggle-btn {
  display: none;
}

/* ============================================
   CONTENT LAYOUT
   ============================================ */
.content {
  margin-left: 220px;
  min-height: 100vh;
  transition: margin-left 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.content.sidebar-collapsed {
  margin-left: 60px;
}

/* ============================================
   RESPONSIVE ADJUSTMENTS
   ============================================ */
@media (max-width: 768px) {
  .nav-side-layout {
    width: 60px;
  }

  .nav-side-layout.collapsed {
    width: 0;
  }

  .content {
    margin-left: 60px;
  }

  .content.sidebar-collapsed {
    margin-left: 0;
  }
}

/* ============================================
   SMOOTH TRANSITIONS FOR CHILD ELEMENTS
   ============================================ */
.full-page {
  min-height: 100vh;
}

/* Ensure content transition is smooth */
.content * {
  transition: none; /* Prevent child elements from animating during sidebar toggle */
}
</style>
