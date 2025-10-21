# Android Build Setup

## Google Services Configuration

This project uses Google services for Android functionality. You need to set up your own Google services configuration.

### Setup Instructions:

1. **Go to the [Firebase Console](https://console.firebase.google.com/)**
2. **Create a new project** or select an existing one
3. **Add an Android app** to your project:
   - Package name: `com.YOUR_COMPANY.WFS_DIY` (replace with your package name)
   - App nickname: `WFS-DIY`
4. **Download the `google-services.json` file**
5. **Place the file** in `Builds/Android/app/src/main/assets/`
6. **Rename the template**: 
   ```bash
   mv Builds/Android/app/src/main/assets/google-services.json.template Builds/Android/app/src/main/assets/google-services.json
   ```
   Then edit it with your actual values.

### Important Security Notes:

- ⚠️ **Never commit your actual `google-services.json` file to version control**
- ✅ **Use the template file** (`google-services.json.template`) as a starting point
- ✅ **Add your own API keys and configuration** locally
- ✅ **The template file is safe to commit** as it contains no real credentials

### File Structure:
```
Builds/Android/app/src/main/assets/
├── google-services.json          # Your actual config (DO NOT COMMIT)
├── google-services.json.template # Template for other developers (SAFE TO COMMIT)
└── ...other assets...
```

## Building the Android App

1. Open the project in Android Studio
2. Ensure you have the `google-services.json` file in place
3. Build and run the project

## Troubleshooting

If you get Google services errors:
- Verify your `google-services.json` file is in the correct location
- Check that the package name matches your app configuration
- Ensure your Firebase project is properly configured
