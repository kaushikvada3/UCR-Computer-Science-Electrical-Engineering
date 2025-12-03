$ErrorActionPreference = "Stop"
$commits = @("cb45527", "90e4970", "c1317e9", "774dfe9", "2aff1b8")
$final_commits = @("af4fdcc", "4c43a66")

Write-Host "Starting History Rewrite..."

# Create temp branch
git checkout -b clean_history 5a9db66

# Process bad commits
foreach ($hash in $commits) {
    Write-Host "Processing $hash..."
    git cherry-pick $hash
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Cherry-pick conflict/error on $hash. Attempting to resolve via rm..."
        # If conflict, likely due to file existence. We want to remove the files anyway.
        git rm --ignore-unmatch "test-code.mem" "1234.mem" "MIPS Datapath Lab-CS161.pdf"
        # Checking if anything else is staged?
        # If conflicts are in other files, this is risky. Assuming conflicts only in target files.
        # Auto-resolving conflicts in target files by deletion:
        git checkout --ours -- "test-code.mem" "1234.mem" "MIPS Datapath Lab-CS161.pdf" 2>$null
        git add .
    }
    # Remove files from index
    git rm --cached --ignore-unmatch "test-code.mem" "1234.mem" "MIPS Datapath Lab-CS161.pdf"
    
    # Commit
    git commit --amend --no-edit --allow-empty
}

# Process final commits
foreach ($hash in $final_commits) {
    Write-Host "Picking $hash..."
    git cherry-pick $hash
}

Write-Host "Done. Switch back to main and merge/reset."
