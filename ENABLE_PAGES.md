# Enable GitHub Pages - Quick Guide

## Method 1: Via GitHub Web Interface (Recommended)

1. **Open your repository settings:**
   - Go to: https://github.com/Sapana-Micro-Software/Multi-Model-Agentic-AI/settings/pages
   - Or navigate: Repository → Settings → Pages (left sidebar)

2. **Configure Pages:**
   - Under "Source", select: **Deploy from a branch**
   - Branch: **main**
   - Folder: **/docs**
   - Click **Save**

3. **Wait for deployment:**
   - GitHub will build and deploy your site
   - You'll see a green checkmark when it's ready
   - Usually takes 1-2 minutes

4. **Access your site:**
   - Your site will be available at: https://sapana-micro-software.github.io/multi-model-agentic-ai
   - The URL will be shown in the Pages settings page

## Method 2: Via GitHub CLI (if you have proper permissions)

If you have admin access to the repository, you can try:

```bash
gh api -X PUT repos/Sapana-Micro-Software/Multi-Model-Agentic-AI/pages \
  -f source[branch]=main \
  -f source[path]=/docs
```

However, this may require Pages to be enabled first via the web interface.

## Verification

After enabling, check the deployment status:
- Go to: https://github.com/Sapana-Micro-Software/Multi-Model-Agentic-AI/actions
- Look for "Deploy GitHub Pages" workflow runs
- Green checkmark = successful deployment

## Troubleshooting

If Pages doesn't appear in Settings:
- Make sure you have admin/owner permissions on the repository
- Check that the repository is public (or you have GitHub Pro/Team for private repos)
- Ensure the `/docs` folder exists with `index.html`

## Current Status

✅ Website files are ready in `/docs/index.html`
✅ GitHub Actions workflow is configured (`.github/workflows/pages.yml`)
⏳ Pages needs to be enabled in repository settings

Once enabled, the site will automatically deploy on every push to `main` branch!

