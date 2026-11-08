// SD card, library indexing and SDRAM sample storage.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Sd Functions --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

SdResult Controller::sd_initialize() {
    SdResult sdResult = SD_ERROR;

    while (sdResult != SD_OK) {
        // 1. SD CARD DETECTION & HARDWARE INIT
        if (sd_detect() == SD_OK) {
            MX_SDMMC2_SD_Init();  // Initialize SDMMC2 peripheral hardware
            sd.detect = true;

            if (sdInsertCheck) {
                lcd.drawInitSdReadAlert();  // Notify user that the card is being read
                sdInsertCheck = false;
            }

            // Re-link the FATFS driver to refresh the SDMMC peripheral state
            // FATFS_UnLinkDriver(SDPath);
            // FATFS_LinkDriver(&SD_Driver, SDPath);

            // 2. FILESYSTEM MOUNT & CARD METADATA
            // Mount FatFS, read volume label/serial, and calculate usage statistics
            if ((sd_mount() == SD_OK) && (sd_getLabel() == SD_OK) && (sd_getSpace() == SD_OK)) {
                sd.serialTemp = sd.serial;

                // 3. REQUIRED DIRECTORY VALIDATION
                // Verify mandatory runtime folders in strict dependency order
                if (sd_checkFolderExist("System") == SD_OK) {
                    if (sd_checkFolderExist("Sample") == SD_OK) {
                        if (sd_checkFolderExist("System/File") == SD_OK) {
                            if (sd_checkFolderExist("System/Drumkit") == SD_OK) {
                                if (sd_checkFolderExist("System/Image") == SD_OK) {
                                    if (sd_checkFolderExist("System/Firmware") == SD_OK) {
                                        // 4. REQUIRED PRESET FILE VALIDATION
                                        // Ensure core preset binaries exist before continuing
                                        if ((sd_checkFolderExist("System/Preset") == SD_OK) &&
                                            (sd_checkFileExist("System/Preset/System.rwp") == SD_OK) &&
                                            (sd_checkFileExist("System/Preset/Encoder.rwp") == SD_OK)) {
                                            // 5. UI ASSET PRELOAD (SD -> SDRAM)
                                            // Load startup/menu graphics to RAM for immediate rendering
                                            if ((sd_loadImage("System/Image/Image_Logo.rwi", RAM_IMAGE_LOGO_PALETTE_ADDRESS, RAM_IMAGE_LOGO_DATA_ADDRESS, kImageLogoPalette, kImageLogoWidth, kImageLogoHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Menu_A.rwi", RAM_IMAGE_MENU_A_PALETTE_ADDRESS, RAM_IMAGE_MENU_A_DATA_ADDRESS, kImageMenuAPalette, kImageMenuAWidth, kImageMenuAHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Menu_B.rwi", RAM_IMAGE_MENU_B_PALETTE_ADDRESS, RAM_IMAGE_MENU_B_DATA_ADDRESS, kImageMenuBPalette, kImageMenuBWidth, kImageMenuBHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Layer_A.rwi", RAM_IMAGE_LAYER_A_PALETTE_ADDRESS, RAM_IMAGE_LAYER_A_DATA_ADDRESS, kImageLayerPalette, kImageLayerWidth, kImageLayerHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Layer_B.rwi", RAM_IMAGE_LAYER_B_PALETTE_ADDRESS, RAM_IMAGE_LAYER_B_DATA_ADDRESS, kImageLayerPalette, kImageLayerWidth, kImageLayerHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Icon_Select.rwi", RAM_ICON_SELECT_PALETTE_ADDRESS, RAM_ICON_SELECT_DATA_ADDRESS, kIconPalette, kIconSelectWidth, kIconSelectHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Icon_Play.rwi", RAM_ICON_PLAY_PALETTE_ADDRESS, RAM_ICON_PLAY_DATA_ADDRESS, kIconPalette, kIconPlayWidth, kIconPlayHeight * 8, RGB16) == SD_OK)) {
                                                // Moffe Edition "Gold Rush" (v2.00): the panel and icon images on the
                                                // card carry the factory cyan / magenta / green; swap them in their
                                                // palettes, so any SD card shows the new look.
                                                const struct { uint32_t address; uint16_t count; } kPalettes[] = {
                                                    {RAM_IMAGE_MENU_A_PALETTE_ADDRESS, kImageMenuAPalette}, {RAM_IMAGE_MENU_B_PALETTE_ADDRESS, kImageMenuBPalette},
                                                    {RAM_IMAGE_LAYER_A_PALETTE_ADDRESS, kImageLayerPalette}, {RAM_IMAGE_LAYER_B_PALETTE_ADDRESS, kImageLayerPalette},
                                                    {RAM_ICON_SELECT_PALETTE_ADDRESS, kIconPalette}, {RAM_ICON_PLAY_PALETTE_ADDRESS, kIconPalette}};
                                                for (const auto& pal : kPalettes) {
                                                    RGB16Color* entry = reinterpret_cast<RGB16Color*>(pal.address);
                                                    for (uint16_t i = 0; i < pal.count; i++) entry[i] = vis::recolor(entry[i], kAccentColor, kAccent2Color);
                                                }
                                                sd.ready = true;
                                                sd.getLibrary = true;  // Signal that we can now index the sample libraries
                                                sdResult = SD_OK;
                                            } else {
                                                sdResult = SD_ERROR_SYSTEMFILE;
                                            }
                                        } else {
                                            sdResult = SD_ERROR_PRESETFOLDER;
                                        }
                                    } else {
                                        sdResult = SD_ERROR_FIRMWAREFOLDER;
                                    }
                                } else {
                                    sdResult = SD_ERROR_IMAGEFOLDER;
                                }
                            } else {
                                sdResult = SD_ERROR_DRUMKITFOLDER;
                            }
                        } else {
                            sdResult = SD_ERROR_FILEFOLDER;
                        }
                    } else {
                        sdResult = SD_ERROR_SAMPLEFOLDER;
                    }
                } else {
                    sdResult = SD_ERROR_SYSTEMFOLDER;
                }
            } else {
                sdResult = SD_ERROR_MOUNT;
            }
        } else {
            sd.detect = false;
            sdResult = SD_ERROR_DETECT;
        }

        // 6. ERROR HANDLING & RECOVERY LOOP
        // Keep prompting and waiting for reinsertion/removal until initialization succeeds
        if (sdResult != SD_OK) {
            lcd_drawInitSdAlert(sdResult);  // Inform user of the specific folder/file missing
            sdInsertCheck = true;

            // Wait for user intervention (re-inserting the card)
            if (sdResult == SD_ERROR_DETECT) {
                while (sd_detect() != SD_OK) {  // Loop until card is physically detected
                }
            } else {
                sd_unmount();                   // Clean up partial mount before retry
                while (sd_detect() == SD_OK) {  // Wait for user to pull the card out
                }
            }
            HAL_Delay(500);  // Debounce delay
        }
    }

    // 7. FINAL UI CLEANUP
    // Initialization is complete, clear startup SD warning overlays
    lcd_clearInitSdAlert();
    return sdResult;
}

SdResult Controller::sd_reinitialize() {
    /// @brief Restores SD operation after mount, removal, or card mismatch errors.
    /// Revalidates card identity and required folders/files before resuming runtime access.

    SdResult sdResult = SD_ERROR;

    while (sdResult != SD_OK) {
        // 1. SD CARD RE-DETECTION
        if (sd_detect() == SD_OK) {
            sd.detect = true;

            // Re-link FatFS driver interfaces to refresh SDMMC transport state
            FATFS_UnLinkDriver(SDPath);
            FATFS_LinkDriver(&SD_Driver, SDPath);

            // 2. FILESYSTEM MOUNT & CARD METADATA
            if ((sd_mount() == SD_OK) && (sd_getLabel() == SD_OK) && (sd_getSpace() == SD_OK)) {
                // 2.1 SERIAL ID VALIDATION
                // Ensure the inserted card matches the original session card.
                if (sd.serial == sd.serialTemp) {
                    // 3. REQUIRED DIRECTORY & PRESET VALIDATION
                    // Re-check mandatory system folders and preset files before re-enabling SD use.
                    if (sd_checkFolderExist("System") == SD_OK) {
                        if (sd_checkFolderExist("Sample") == SD_OK) {
                            if (sd_checkFolderExist("System/File") == SD_OK) {
                                if (sd_checkFolderExist("System/Drumkit") == SD_OK) {
                                    if (sd_checkFolderExist("System/Image") == SD_OK) {
                                        if (sd_checkFolderExist("System/Firmware") == SD_OK) {
                                            if ((sd_checkFolderExist("System/Preset") == SD_OK) &&
                                                (sd_checkFileExist("System/Preset/System.rwp") == SD_OK) &&
                                                (sd_checkFileExist("System/Preset/Encoder.rwp") == SD_OK)) {
                                                sd.ready = true;
                                                sd.getLibrary = true;
                                                sdResult = SD_OK;
                                            } else {
                                                sdResult = SD_ERROR_PRESETFOLDER;
                                            }
                                        } else {
                                            sdResult = SD_ERROR_FIRMWAREFOLDER;
                                        }
                                    } else {
                                        sdResult = SD_ERROR_IMAGEFOLDER;
                                    }
                                } else {
                                    sdResult = SD_ERROR_DRUMKITFOLDER;
                                }
                            } else {
                                sdResult = SD_ERROR_FILEFOLDER;
                            }
                        } else {
                            sdResult = SD_ERROR_SAMPLEFOLDER;
                        }
                    } else {
                        sdResult = SD_ERROR_SYSTEMFOLDER;
                    }
                } else {
                    // Serial mismatch: a different SD card was inserted.
                    sdResult = SD_ERROR_SERIAL;
                }
            } else {
                sdResult = SD_ERROR_MOUNT;
            }
        } else {
            sd.detect = false;
            sdResult = SD_ERROR_DETECT;
        }

        // 4. ERROR HANDLING & RECOVERY LOOP
        // Keep user informed and wait for the correct detect/remove action before retrying.
        if (sdResult != SD_OK) {
            lcd_drawSdAlert(sdResult);  // Visual feedback (e.g., "Insert Same SD Card")

            if (sdResult == SD_ERROR_DETECT) {
                while (sd_detect() != SD_OK) {
                    lcd_update();  // Keep the UI responsive while waiting for card
                    Watchdog_Kick();  // v2.09: waiting for the user is not a hang
                }
            } else {
                sd_unmount();
                while (sd_detect() == SD_OK) {
                    lcd_update();  // Wait for user to remove the "wrong" card
                    Watchdog_Kick();
                }
            }
            HAL_Delay(500);
        }
    }

    // 5. POST-RECOVERY LIBRARY REBUILD
    // Card is valid again; rebuild project/drumkit/sample library caches.
    lcd_drawSdAlert(SD_ANALYZE);
    sd_getLibraries();
    lcd_clearSdAlert();

    // 6. UI STATE RESTORATION
    // Clear alert mode and return to normal global screen flow.
    alertFlag = false;
    alertType = ALERT_OFF;
    global_select();

    return sdResult;
}

SdResult Controller::sd_detect() {
    /// @brief Detects physical SD card presence using the dedicated detect pin.
    /// Reads GPIO IDR directly and applies active-low card-detect logic.
    /// @note The detect line is asserted low when a card is inserted.

    if (!(SDMMC2_DETECT_GPIO_Port->IDR & SDMMC2_DETECT_Pin)) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_mount() {
    /// @brief Mounts the FatFS volume and registers the active work area.
    /// Returns SD_OK only when f_mount completes successfully.

    if (f_mount(&sd.fs, SDPath, 0) == FR_OK) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_unmount() {
    /// @brief Unmounts the FatFS volume and unregisters the active work area.
    /// Passes nullptr to f_mount to detach the file system context.

    if (f_mount(nullptr, SDPath, 0) == FR_OK) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_getLabel() {
    /// @brief Reads SD volume label and persistent serial identifier.
    /// Serial metadata is used for card-identity validation during recovery flows.

    if (f_getlabel(SDPath, sd.label, &sd.serial) == FR_OK) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_setLabel() {
    /// @brief Sets the SD volume label to the expected system name.
    /// Writes the fixed label "DRUMBOY" to the mounted file system.

    if (f_setlabel("DRUMBOY") == FR_OK) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_getSpace() {
    /// @brief Computes SD capacity statistics (total, free, used) in MiB.

    FATFS* fs_ptr = &sd.fs;
    DWORD freeCluster;

    // 1. FREE CLUSTER QUERY
    // Request current free-cluster count from the mounted FatFS volume.
    if (f_getfree(SDPath, &freeCluster, &fs_ptr) != FR_OK) {
        sd.totalSpace = 0;
        sd.freeSpace = 0;
        sd.usedSpace = 0;
        return SD_ERROR;
    }

    // 2. SECTOR COUNT DERIVATION
    // FatFS n_fatent stores FAT entry count; subtract reserved entries.
    const uint32_t totalSectors = (static_cast<uint32_t>(sd.fs.n_fatent) - 2) * sd.fs.csize;
    const uint32_t freeSectors = freeCluster * sd.fs.csize;

    // 3. MiB CONVERSION & CACHE UPDATE
    // For 512-byte sectors, 2048 sectors correspond to 1 MiB.
    sd.totalSpace = static_cast<uint16_t>(totalSectors / 2048);
    sd.freeSpace = static_cast<uint16_t>(freeSectors / 2048);
    sd.usedSpace = sd.totalSpace - sd.freeSpace;

    return SD_OK;
}

SdResult Controller::sd_checkFileExist(const char* fileAddress) {
    /// @brief Checks whether a regular file exists at the given SD path.

    // f_stat must succeed and AM_DIR must be clear (entry is not a directory).
    if ((f_stat(fileAddress, &sd.fileInfo) == FR_OK) &&
        !(sd.fileInfo.fattrib & AM_DIR)) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_checkFolderExist(const char* folderAddress) {
    /// @brief Checks whether a directory exists at the given SD path.

    // f_stat must succeed and AM_DIR must be set (entry is a directory).
    if ((f_stat(folderAddress, &sd.fileInfo) == FR_OK) &&
        (sd.fileInfo.fattrib & AM_DIR)) {
        return SD_OK;
    }
    return SD_ERROR;
}

SdResult Controller::sd_loadImage(char* fileAddress, uint32_t paletteAddress, uint32_t dataAddress, uint16_t paletteSize, uint16_t width, uint16_t height, RGBMode mode) {
    /// @brief Loads a custom RW image from SD directly into RAM palette/pixel buffers.
    /// Validates signature, metadata, and section markers before accepting image payload data.

    // 1. INPUT VALIDATION
    if (!fileAddress) return SD_ERROR;
    SdResult result = SD_ERROR;
    UINT bytesRead;

    // 2. HEADER BUFFER PREPARATION
    // Local scratch buffers for file signature, markers, and metadata fields.
    char signature[11];  // File ID: "RW_IMAGE  "
    char marker[4];      // Data markers: "_PA" (Palette) or "_DA" (Data)
    uint16_t meta[4];    // [palette_size, rgb_depth, width, height]

    // 3. INPUT PROTECTION
    // Disable keyboard interrupts to avoid bus/timing conflicts during SD reads.
    keyboard_disable();

    // 4. FILE PARSING & LOAD PIPELINE
    if (f_open(&sd.file, fileAddress, FA_READ) == FR_OK) {
        // 4.1 SIGNATURE VALIDATION
        if (f_read(&sd.file, signature, 10, &bytesRead) == FR_OK && strncmp(signature, "RW_IMAGE  ", 10) == 0) {
            // 4.2 METADATA VALIDATION
            if (sd_read(meta, 8) == FR_OK) {
                const uint16_t expectedDepth = (mode == RGB24) ? 24 : 16;

                // Compare file header against requested UI parameters
                if (meta[0] == paletteSize && meta[1] == expectedDepth && meta[2] == width && meta[3] == height) {
                    // 4.3 PALETTE SECTION LOAD
                    if (f_read(&sd.file, marker, 3, &bytesRead) == FR_OK &&
                        strncmp(marker, "_PA", 3) == 0) {
                        const uint32_t paletteByteSize = paletteSize * ((mode == RGB24) ? 3 : 2);

                        // Direct read into RAM at the palette pointer
                        if (sd_read(reinterpret_cast<void*>(paletteAddress), paletteByteSize) == FR_OK) {
                            // 4.4 PIXEL SECTION LOAD
                            if (f_read(&sd.file, marker, 3, &bytesRead) == FR_OK &&
                                strncmp(marker, "_DA", 3) == 0) {
                                const uint32_t pixelByteSize = width * height;  // 1 byte per pixel (indexed)

                                // Direct read into RAM at the pixel buffer pointer
                                if (f_read(&sd.file, reinterpret_cast<void*>(dataAddress), pixelByteSize, &bytesRead) == FR_OK) {
                                    if (bytesRead == pixelByteSize) {
                                        result = SD_OK;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 5. FILE RESOURCE CLEANUP
        // Close file descriptor before leaving the function scope.
        f_close(&sd.file);
    }

    // 6. INPUT RESTORATION
    // Re-enable keyboard interrupts once SD transaction flow is complete.
    keyboard_enable();
    return result;
}

void Controller::sd_getLibraries() {
    /// @brief Rebuilds all SD-backed runtime library caches in a deterministic sequence.
    /// Refreshes project, drumkit, instrument, and sample indexes before integrity validation.

    // 1. CACHE DIRECTORY RESET
    // Remove temporary index artifacts to force a clean rebuild pass.
    sd_deleteDirectory("System/Data");

    // 2. PROJECT LIBRARY INDEXING
    // Scan and register available project slots.
    sd_getFileLibrary();

    // 3. DRUMKIT LIBRARY INDEXING
    // Scan and register available drumkit entries.
    sd_getDrumkitLibrary();

    // 4. INSTRUMENT LIBRARY INDEXING
    // Build the instrument-folder map used by sample scanning.
    sd_getInstLibrary();

    // 5. SAMPLE LIBRARY INDEXING
    // Index sample files for each discovered instrument.
    sd_getSampleLibrary();

    // 6. ACTIVE SAMPLE INTEGRITY VALIDATION
    // Cross-check active layer assignments against refreshed sample indexes.
    sd_checkSamplesInUse();
}

SdResult Controller::sd_getFileLibrary() {
    /// @brief Indexes existing project files and counts valid entries marked with an asterisk.
    /// Scans the 'System/File' directory using a structured slot-based naming convention.

    SdResult result = SD_ERROR;
    char refRead[sizeof(kFileRef)] = {};
    char data;

    // 1. INPUT PROTECTION
    // Disable keyboard interrupts to prevent I2C/DMA collisions during heavy SD card bus activity.
    keyboard_disable();
    fileLibrarySize = 0;

    // 2. SLOT SCANNING LOOP
    // Iterate through all possible file slots defined by the kFileLibrarySize constant.
    for (uint8_t i = 0; i < kFileLibrarySize; i++) {
        char fileName[50];
        char fileNum[3];

        // 3. FILENAME CONSTRUCTION
        // Generate a formatted 3-digit number (e.g., 001, 002) to create the full path.
        sprintf(fileNum, "%03d", (i + 1));
        strcpy(fileName, kFileStart);
        strcat(fileName, fileNum);
        strcat(fileName, kFileEnd);

        // 4. HEADER VALIDATION
        // Attempt to open the project file. If successful, verify the file header and occupancy marker.
        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            // Read the unique file reference string (kFileRef) to verify compatibility.
            if (sd_read(refRead, strlen(kFileRef)) == 0) {
                // Read the single character status marker immediately following the header.
                if (sd_read(&data, 1) == FR_OK) {
                    // 5. VALIDITY CHECK
                    // An asterisk '*' indicates that this project slot contains active user data.
                    if (data == '*') {
                        fileLibrarySize += 1;
                    }
                }
            }
        }

        // 6. RESOURCE CLEANUP
        // Ensure the file is closed before the next iteration to avoid FatFS handle overflows.
        f_close(&sd.file);
    }

    result = SD_OK;

    // 7. INPUT RESTORATION
    // Re-enable keyboard interrupts once the SD card scan is complete.
    keyboard_enable();

    return result;
}

SdResult Controller::sd_getDrumkitLibrary() {
    /// @brief Indexes existing drumkit presets and counts valid entries marked with an asterisk.
    /// Scans the 'System/Drumkit' directory for available drumkit slots.

    SdResult result = SD_ERROR;
    char refRead[sizeof(kDrumkitRef)] = {};
    char data;

    // 1. INPUT PROTECTION
    // Disable keyboard interrupts to prevent I2C bus collisions during heavy SD read operations.
    keyboard_disable();
    drumkitLibrarySize = 0;

    // 2. SLOT-BASED SCANNING LOOP
    // Iterate through all possible drumkit slots defined by kDrumkitLibrarySize.
    for (uint8_t i = 0; i < kDrumkitLibrarySize; i++) {
        char fileName[50];
        char fileNum[3];

        // 3. FILENAME GENERATION
        // Format the current index into a 3-digit string (e.g., Drumkit_001.rwd).
        sprintf(fileNum, "%03d", (i + 1));
        strcpy(fileName, kDrumkitStart);
        strcat(fileName, fileNum);
        strcat(fileName, kDrumkitEnd);

        // 4. HEADER VALIDATION
        // Attempt to open the drumkit file. If it exists, verify its internal marker.
        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            // Read the reference header string to ensure file type compatibility.
            if (sd_read(refRead, strlen(kDrumkitRef)) == 0) {
                // Read the occupancy marker character immediately after the header.
                if (sd_read(&data, 1) == FR_OK) {
                    // 5. VALIDITY CHECK
                    // An asterisk '*' confirms this slot is active and contains user data.
                    if (data == '*') {
                        drumkitLibrarySize += 1;
                    }
                }
            }
        }

        // 6. RESOURCE CLEANUP
        // Close the file handle before the next iteration to avoid reaching the FatFS handle limit.
        f_close(&sd.file);
    }

    result = SD_OK;

    // 7. INPUT RESTORATION
    // Re-enable keyboard interrupts once the SD card bus is idle.
    keyboard_enable();

    return result;
}

void Controller::sd_getInstLibrary() {
    /// @brief Indexes instrument folders under 'Sample/' and generates a cached instrument library file.
    /// Uses SDRAM-backed name storage and DMA-safe aligned buffers for reliable SD writes.

    instLibrarySize = 0;

    // Base address for name strings
    char* ramPtr = (char*)RAM_INST_ADDRESS;

    // Pointer array for alphabetical sorting logic
    char** sortArray = (char**)(RAM_INST_ADDRESS + (kInstLibraryMaxSize * kFileNameSize));

    // 32-byte aligned buffer specifically for SDMMC DMA safety
    static char safeBuffer[kFileNameSize] __attribute__((aligned(32)));

    // 1. INPUT PROTECTION
    // Lock interrupts to prevent I2C/DMA bus contention during the scan.
    keyboard_disable();

    // 2. ROOT DIRECTORY SCAN PIPELINE
    // Open Sample root and collect valid instrument folders into SDRAM.
    if (f_opendir(&sd.dir, "/Sample") == FR_OK) {
        while (instLibrarySize < kInstLibraryMaxSize) {
            Watchdog_Kick();  // v2.09: a big card can take a while to index
            // 2.1 ENTRY READING
            // Fetch the next directory item; exit if end of folder or error.
            if (f_readdir(&sd.dir, &sd.fileInfo) != FR_OK || sd.fileInfo.fname[0] == 0) {
                break;
            }

            // 2.2 FOLDER VALIDATION
            // Filter for valid, non-hidden directories within the allowed filename length.
            if ((sd.fileInfo.fattrib & AM_DIR) &&
                !(sd.fileInfo.fattrib & AM_HID) &&
                (strlen(sd.fileInfo.fname) < kFileNameSize)) {
                // 2.3 SDRAM CACHE STORAGE
                // Calculate the destination pointer and copy the filename into SDRAM.
                char* entryPtr = ramPtr + (instLibrarySize * kFileNameSize);
                memset(entryPtr, 0, kFileNameSize);
                strncpy(entryPtr, sd.fileInfo.fname, kFileNameSize - 1);

                // Store pointer address for the sorting algorithm
                sortArray[instLibrarySize] = entryPtr;
                instLibrarySize++;
            }
        }
        f_closedir(&sd.dir);
    }

    // 6. ALPHABETICAL SORTING
    // Reorganize the pointer array so the UI displays folders in A-Z order.
    if (instLibrarySize > 0) {
        sortWords(sortArray, instLibrarySize);
    }

    // 7. CACHE DIRECTORY VERIFICATION
    // Ensure the System/Data path exists before attempting to write the library file.
    if (f_stat("System/Data", &sd.fileInfo) != FR_OK) {
        f_mkdir("System/Data");
    }

    // 8. LIBRARY FILE GENERATION
    // Create 'Inst.lib' to store the sorted index for fast subsequent loads.
    if (f_open(&sd.file, "System/Data/Inst.lib", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        // 9. HEADER SIGNATURE
        // Write the custom file identity string.
        const char header[] = "RW_DRUMBOY_INST          ";
        sd_write(header, 25);

        // 10. DMA-SAFE BUFFERED DATA WRITE
        // Loop through sorted pointers, copying to aligned SRAM before writing to SD.
        for (uint16_t i = 0; i < instLibrarySize; i++) {
            char* sourcePtr = sortArray[i];
            if (sourcePtr == nullptr) continue;

            // Step copy: SDRAM -> Aligned SRAM (CPU) -> SD Card (DMA)
            memcpy(safeBuffer, sourcePtr, kFileNameSize);
            sd_write(safeBuffer, kFileNameSize);
        }

        // 11. EOF MARKER
        // Finalize the file structure with an End-Of-File identifier.
        sd_write("EOF", 3);
        f_close(&sd.file);
    }

    // 12. UI FEEDBACK & RESTORATION
    // Draw a progress indicator and re-enable input hardware.
    lcd.setForeColor(BLACK);
    lcd.drawText("1", 1, 50, 50);
    keyboard_enable();
}

void Controller::sd_getSampleLibrary() {
    /// @brief Indexes WAV samples per instrument and builds per-instrument metadata cache files (.ins).
    /// Consumes the instrument master index and writes sorted sample entries using DMA-safe buffering.

    FIL readFil;
    char instName[kFileNameSize];
    char pathBuffer[128];

    // Pointer array in SDRAM for sorting; offset to avoid overwriting current sample names
    char** sortArray = (char**)(RAM_SAMPLE_ADDRESS + (kInstSampleLibraryMaxSize * kFileNameSize));

    // 32-byte aligned SRAM buffer for SDMMC DMA cache coherency
    static char safeBuffer[kFileNameSize] __attribute__((aligned(32)));

    totalSampleLibrarySize = 0;

    // 1. INPUT PROTECTION
    // Lock keyboard interrupts to prevent I2C/SD bus contention during high-latency operations.
    keyboard_disable();

    // 2. MASTER INDEX ACCESS
    // Open the previously created instrument library file to read the folder list.
    if (f_open(&readFil, "System/Data/Inst.lib", FA_READ) == FR_OK) {
        // 3. HEADER NAVIGATION
        // Skip the 'RW_DRUMBOY_INST' signature (25 bytes) to reach the first entry.
        f_lseek(&readFil, 25);

        // 4. INSTRUMENT ITERATION
        // Process each indexed instrument folder discovered during the previous scan.
        for (uint16_t i = 0; i < instLibrarySize; i++) {
            UINT bytesRead;
            if (f_read(&readFil, instName, kFileNameSize, &bytesRead) != FR_OK || bytesRead < kFileNameSize) {
                break;
            }
            instName[kFileNameSize - 1] = 0;  // Enforce null-termination
            if (instName[0] == 0) continue;

            sampleLibrarySize[i] = 0;

            // 5. SAMPLE DIRECTORY SCAN
            // Open the specific category folder (e.g., 'Sample/Kicks') on the SD card.
            snprintf(pathBuffer, sizeof(pathBuffer), "Sample/%s", instName);
            if (f_opendir(&sd.dir, pathBuffer) == FR_OK) {
                while (sampleLibrarySize[i] < kInstSampleLibraryMaxSize) {
                    Watchdog_Kick();
                    if (f_readdir(&sd.dir, &sd.fileInfo) != FR_OK || sd.fileInfo.fname[0] == 0) {
                        break;
                    }

                    // 5.1 FILE FILTERING
                    // Only accept physical files (not directories) with a valid '.wav' extension.
                    if (!(sd.fileInfo.fattrib & AM_DIR) && !(sd.fileInfo.fattrib & AM_HID)) {
                        char* fname = sd.fileInfo.fname;
                        int len = strlen(fname);

                        if (len > 4 && strcasecmp(&fname[len - 4], ".wav") == 0 && len < kFileNameSize) {
                            // 5.2 SDRAM CACHE STORAGE
                            // Copy the filename into External SDRAM for sorting and metadata generation.
                            char* entryPtr = (char*)(RAM_SAMPLE_ADDRESS + (sampleLibrarySize[i] * kFileNameSize));
                            memset(entryPtr, 0, kFileNameSize);
                            strncpy(entryPtr, fname, kFileNameSize - 1);

                            sortArray[sampleLibrarySize[i]] = entryPtr;
                            sampleLibrarySize[i]++;
                        }
                    }
                }
                f_closedir(&sd.dir);
            }

            // 8. ALPHABETICAL SORTING
            // Organize samples (e.g., Kick_01, Kick_02) so they appear logically in the UI.
            if (sampleLibrarySize[i] > 0) {
                sortWords(sortArray, sampleLibrarySize[i]);
            }

            // 9. METADATA FILE GENERATION
            // Create a specialized .ins file to cache the sorted list for this instrument.
            snprintf(pathBuffer, sizeof(pathBuffer), "System/Data/%s.ins", instName);
            if (f_open(&sd.file, pathBuffer, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
                // 10. SIGNATURE WRITE
                // Mark the file with the 'RW_DRUMBOY_SAMPLE' header.
                const char header[] = "RW_DRUMBOY_SAMPLE        ";
                sd_write(header, 25);

                // 11. DMA-SAFE DATA BUFFERING
                // Copy data from SDRAM to aligned SRAM before the SDMMC write to ensure stability.
                for (uint16_t j = 0; j < sampleLibrarySize[i]; j++) {
                    char* sourcePtr = sortArray[j];
                    if (sourcePtr == nullptr) continue;

                    memcpy(safeBuffer, sourcePtr, kFileNameSize);
                    sd_write(safeBuffer, kFileNameSize);
                }

                // 12. EOF TERMINATION
                // Close the metadata file with an EOF marker.
                sd_write("EOF", 3);
                f_close(&sd.file);
            }
            totalSampleLibrarySize += sampleLibrarySize[i];
        }
        f_close(&readFil);
    }

    // 13. INPUT RESTORATION & UI FEEDBACK
    // Draw a progress indicator and re-enable input hardware.
    lcd.setForeColor(BLACK);
    lcd.drawText("2", 1, 50, 50);
    keyboard_enable();
}

SdResult Controller::sd_checkFile(uint8_t fileNum_) {
    /// @brief Validates a project file's existence and internal status before loading.
    /// Categorizes the slot to prevent loading empty or corrupt project data.

    SdResult result = SD_ERROR;
    fileStatus = FILE_NONE;
    if (fileNum_ != kAutosaveFileNum) pname::clear(fileSlotName);
    char refRead[sizeof(kFileRef)] = {};
    char data;
    char fileName[50];

    // 1. FILENAME CONSTRUCTION
    // File_NNN.rwf for a slot, Autosave.rwf for kAutosaveFileNum.
    projectFileName(fileNum_, fileName, sizeof(fileName));

    // 2. FILE ACCESS
    // Attempt to open the file. If it fails, the file is physically missing.
    if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
        // 3. SIGNATURE VERIFICATION
        // Read the file header and compare it against the global kFileRef string.
        if (sd_read(refRead, strlen(kFileRef)) == FR_OK) {
            if (strncmp(kFileRef, refRead, strlen(kFileRef)) == 0) {
                // 4. OCCUPANCY CHECK
                // If the signature matches, read the status byte. '*' denotes a saved project.
                sd_read(&data, 1);
                (data == '*') ? fileStatus = FILE_ACTIVE : fileStatus = FILE_INACTIVE;

                // v2.02: the slot's name for the File menu (the auto-save is never shown there).
                if ((fileStatus == FILE_ACTIVE) && (fileNum_ != kAutosaveFileNum)) {
                    uint8_t nameBytes[pname::kLength];
                    if ((f_lseek(&sd.file, strlen(kFileRef) + pname::kNameOffset) == FR_OK) && (sd_read(nameBytes, pname::kLength) == FR_OK)) {
                        pname::decode(nameBytes, fileSlotName);
                    }
                }
            } else {
                // 5. COMPATIBILITY HANDLING
                // The file exists but the header is unrecognized or from an older version.
                fileStatus = FILE_INCOMPATIBLE;
            }
        } else {
            fileStatus = FILE_INCOMPATIBLE;
        }
    } else {
        // 6. ERROR STATE ASSIGNMENT
        // The file could not be found in the expected directory.
        fileStatus = FILE_MISSING;
    }

    // 7. RESOURCE CLEANUP
    // Always close the file handle to free up FatFS resources for the next operation.
    f_close(&sd.file);
    result = SD_OK;
    return result;
}

SdResult Controller::sd_loadFile(uint8_t fileNum_) {
    /// @brief Loads a project file and reconstructs runtime state in two verified phases.
    /// Applies global parameters, validates instrument/sample references, and restores song beats.

    SdResult result = SD_ERROR;
    char data[1500] = {};
    char fileName[50];

    // 1. FILENAME CONSTRUCTION
    // File_NNN.rwf for a slot, Autosave.rwf for kAutosaveFileNum.
    projectFileName(fileNum_, fileName, sizeof(fileName));

    char instName[kLayerLibrarySize][kFileNameSize + 1];
    uint16_t instNum[kLayerLibrarySize];
    bool instActive[kLayerLibrarySize];
    bool instMissing[kLayerLibrarySize];
    char sampleName[kLayerLibrarySize][kFileNameSize + 1];
    uint16_t sampleNum[kLayerLibrarySize];
    bool sampleActive[kLayerLibrarySize];
    bool sampleMissing[kLayerLibrarySize];
    bool phaseCheck[2] = {false};

    // 2. INPUT PROTECTION
    // Disable keyboard interrupts to avoid bus contention during SD file I/O.
    keyboard_disable();

    // 3. SD STATE RECOVERY
    // Ensure stale file handles are closed and abort unfinished SD transactions.
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
        HAL_SD_Abort(&hsd2);
    }

    // 4. FILE ELIGIBILITY CHECK
    // Load only if file exists and is marked as an active project slot.
    if ((sd_checkFile(fileNum_) == SD_OK) && (fileStatus == FILE_ACTIVE)) {
        // 4.1 PREVIOUS SONG CLEANUP
        // Clear every layer's existing beats before this file's song values (bar/measure/etc.) are applied.
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            for (uint8_t j = 0; j < kBankLibrarySize; j++) {
                layerSong_resetAllBeats(i, j);
            }
        }

        // 5. PHASE-1 LOAD: DRUMKIT + ASSET REFERENCES
        // Read and decode project parameters, then resolve instrument/sample links.
        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            if (f_lseek(&sd.file, strlen(kFileRef)) == FR_OK) {
                const uint16_t bufferByteSize = 512;
                const uint16_t readByteSize = kDrumkitByteSize;
                uint16_t readCountTotal = (readByteSize / bufferByteSize);
                uint16_t remainderByteSize = (readByteSize % bufferByteSize);
                uint8_t dataRead[bufferByteSize] = {0};
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                // 5.1 BLOCK READ
                // Read fixed-size 512-byte chunks for efficient SD throughput.
                for (uint16_t i = 0; i < readCountTotal; i++) {
                    if (sd_read(&dataRead, bufferByteSize) != FR_OK) errorCount += 1;
                    for (uint16_t j = 0; j < bufferByteSize; j++) {
                        volatile uint8_t* writePtr = (volatile uint8_t*)(data + pointerOffset);
                        *writePtr = dataRead[j];
                        pointerOffset += 1;
                    }
                    memset(dataRead, 0x00, bufferByteSize);
                }

                // 5.2 TAIL READ
                // Read the final partial block when payload is not aligned to 512 bytes.
                if (remainderByteSize) {
                    if (sd_read(&dataRead, remainderByteSize) != FR_OK) errorCount += 1;
                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                        volatile uint8_t* writePtr = (volatile uint8_t*)(data + pointerOffset);
                        *writePtr = dataRead[k];
                        pointerOffset += 1;
                    }
                }
                f_close(&sd.file);
                if (errorCount == 0) {
                    // 5.3 PARAMETER DECODE
                    // Decode and apply mixer/rhythm/FX/layer parameters from the buffer.

                    // Mixer configuration

                    // Rhythm configuration
                    rhythm_setTempo(data[13]);
                    rhythm_setMeasure(data[14]);
                    rhythm_setBar(data[15]);
                    rhythm_setQuantize(data[16]);

                    // Song mode chain (v1.07+). Files from older firmware hold zeros here,
                    // which decode to the default chain with song mode OFF.
                    {
                        uint8_t chainBytes[songchain::kEncodedSize];
                        chainBytes[0] = static_cast<uint8_t>(data[kSongChainHeaderOffset]);
                        for (uint8_t i = 0; i < songchain::kMaxSteps; i++) chainBytes[1 + i] = static_cast<uint8_t>(data[kSongChainStepsOffset + i]);
                        if (songchain::decode(chainBytes, songChain)) {
                            // v2.03: TRANSPOSE per step (zeros in older files).
                            songchain::decodeTranspose(reinterpret_cast<const uint8_t*>(&data[kSongTransposeOffset]), songChain);
                        }
                        song_onFileLoaded();
                    }

                    // v2.02: the project's name (none in files from older firmware).
                    pname::decode(reinterpret_cast<const uint8_t*>(&data[pname::kNameOffset]), projectName);

                    // AUTOSAVE (v2.01): the slot the auto-saved session came from.
                    if (fileNum_ == kAutosaveFileNum) {
                        const uint8_t slot = static_cast<uint8_t>(data[kAutosaveOriginOffset + 1]);
                        currentFileSlot = ((data[kAutosaveOriginOffset] == 'S') && (slot >= 1) && (slot <= kFileLibrarySize)) ? static_cast<int8_t>(slot - 1) : -1;
                    }

                    // Metronome settings

                    // IO settings

                    // Equalizer settings
                    eq_setActive((bool)data[25]);
                    eq_setQ(data[26]);
                    eq_setFreqLowShelf(data[27]);
                    eq_setGainLowShelf(data[28]);
                    eq_setFreqHighShelf(data[29]);
                    eq_setGainHighShelf(data[30]);
                    eq_setFreqPeak(0, data[31]);
                    eq_setFreqPeak(1, data[32]);
                    eq_setFreqPeak(2, data[33]);
                    eq_setFreqPeak(3, data[34]);
                    eq_setGainPeak(0, data[35]);
                    eq_setGainPeak(1, data[36]);
                    eq_setGainPeak(2, data[37]);
                    eq_setGainPeak(3, data[38]);

                    // Filter configuration (0 and 1)
                    filter_setActive(0, (bool)data[39]);
                    filter_setType(0, data[40]);
                    filter_setFrequency(0, data[41]);
                    filter_setResonance(0, data[42]);
                    filter_setSlope(0, data[43]);
                    filter_setDrive(0, data[44]);
                    filter_setDry(0, data[45]);
                    filter_setWet(0, data[46]);

                    filter_setActive(1, (bool)data[47]);
                    filter_setType(1, data[48]);
                    filter_setFrequency(1, data[49]);
                    filter_setResonance(1, data[50]);
                    filter_setSlope(1, data[51]);
                    filter_setDrive(1, data[52]);
                    filter_setDry(1, data[53]);
                    filter_setWet(1, data[54]);

                    // Effect processor settings (A/B and subeffects)
                    effect_setActive(0, (bool)data[55]);
                    effect_setType(0, data[56]);
                    effect_setActive(1, (bool)data[57]);
                    effect_setType(1, data[58]);
                    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
                        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
                            uint16_t baseNum = 59 + (54 * i) + (6 * j);
                            effect_setAData(i, j, data[baseNum + 0]);
                            effect_setBData(i, j, data[baseNum + 1]);
                            effect_setCData(i, j, data[baseNum + 2]);
                            effect_setDData(i, j, data[baseNum + 3]);
                            effect_setEData(i, j, data[baseNum + 4]);
                            effect_setFData(i, j, data[baseNum + 5]);
                        }
                    }

                    // Reverb processor parameters
                    reverb_setActive(data[167]);
                    reverb_setSize(data[168]);
                    reverb_setDecay(data[169]);
                    reverb_setDamping(data[170]);
                    reverb_setPreDelay(data[171]);
                    reverb_setSurround(data[172]);
                    reverb_setDry(data[173]);
                    reverb_setWet(data[174]);

                    // LFO modulation settings
                    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
                        uint16_t offset = 180 + (i * 6);
                        lfo_setActive(i, (bool)data[offset + 0]);
                        lfo_setType(i, data[offset + 1]);
                        lfo_setRate(i, data[offset + 2]);
                        lfo_setPhase(i, data[offset + 3]);
                        lfo_setInvert(i, data[offset + 4]);
                        lfo_setDepth(i, data[offset + 5]);
                    }

                    // Layer instrument configuration
                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                        uint16_t offset = 240 + (i * 25);
                        layerInst_setLevel(i, data[offset + 0]);
                        layerInst_setPitch(i, data[offset + 1]);
                        layerInst_setDirection(i, data[offset + 2]);
                        layerInst_setResolution(i, data[offset + 3]);
                        layerInst_setStart(i, data[offset + 4]);
                        layerInst_setEnd(i, data[offset + 5]);
                        layerInst_setTiming(i, data[offset + 6]);
                        layerInst_setProbability(i, data[offset + 7]);
                        layerInst_setLfoLevel(i, data[offset + 8]);
                        layerInst_setLfoPitch(i, data[offset + 9]);
                        layerInst_setLfoDirection(i, data[offset + 10]);
                        layerInst_setLfoResolution(i, data[offset + 11]);
                        layerInst_setLfoStart(i, data[offset + 12]);
                        layerInst_setLfoEnd(i, data[offset + 13]);
                        layerInst_setLfoTiming(i, data[offset + 14]);
                        layerInst_setLfoProbability(i, data[offset + 15]);
                        layerInst_setEq(i, data[offset + 16]);
                        layerInst_setFilter(i, data[offset + 17]);
                        layerInst_setEffect(i, data[offset + 18]);
                        layerInst_setReverb(i, data[offset + 19]);
                        layerInst_setMute(i, (bool)data[offset + 20]);
                        layerInst_setFill(i, (bool)data[offset + 21]);
                        layerInst_setStyle(i, (bool)data[offset + 22]);
                    }

                    // 5.4 ASSET REFERENCE EXTRACTION
                    // Extract instrument/sample names and activation markers per layer.
                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                        Layer& layer = layerLibrary[i];
                        uint16_t layerOffset = 500 + (i * 100);
                        (data[layerOffset] == 'x') ? instActive[i] = true : instActive[i] = false;
                        (data[layerOffset + 50] == 'x') ? sampleActive[i] = true : sampleActive[i] = false;
                        memcpy(instName[i], &(data[layerOffset + 1]), kFileNameSize + 1);
                        memcpy(sampleName[i], &(data[layerOffset + 50 + 1]), kFileNameSize + 1);
                    }

                    // 5.5 LIBRARY CROSS-CHECK
                    // Match extracted references against generated libraries and mark missing assets.
                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                        // Instrument lookup in master index
                        if (instActive[i]) {
                            if ((f_open(&sd.file, "System/Data/Inst.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, 25) == FR_OK)) {
                                char searchName[kFileNameSize + 1];
                                bool searchActive = true;
                                uint16_t searchCounter = 0;
                                while (searchActive) {
                                    memset(searchName, 0x00, sizeof(searchName));
                                    sd_read(searchName, kFileNameSize);
                                    if (strcmp(instName[i], searchName) == 0) {
                                        instNum[i] = searchCounter;
                                        instMissing[i] = false;
                                        searchActive = false;
                                    } else if (searchCounter >= instLibrarySize) {
                                        instNum[i] = -1;
                                        instMissing[i] = true;
                                        searchActive = false;
                                    }
                                    searchCounter += 1;
                                }
                            } else {
                                instMissing[i] = true;
                            }
                            f_close(&sd.file);
                        } else {
                            instNum[i] = -1;
                            instMissing[i] = false;
                        }

                        // Apply instrument selection state to layer
                        if (instMissing[i]) {
                            layerInst_setInstSelected(i, -1);
                            layerInst_setInstLoaded(i);
                            layerInst_setSampleSelected(i, -1);
                            layerInst_setSampleLoaded(i);
                        } else {
                            layerInst_setInstSelected(i, instNum[i]);
                            layerInst_setInstLoaded(i);
                        }

                        // Sample lookup in per-instrument index
                        if ((sampleActive[i]) && (instActive[i]) && (!instMissing[i])) {
                            char instFileName[kFileNameSize + 15] = "System/Data/";
                            strncat(instFileName, instName[i], kFileNameSize);
                            strncat(instFileName, ".ins", 4);
                            if ((f_open(&sd.file, instFileName, FA_READ) == FR_OK) && (f_lseek(&sd.file, 25) == FR_OK)) {
                                char searchName[kFileNameSize + 1];
                                bool searchActive = true;
                                uint16_t searchCounter = 0;
                                while (searchActive) {
                                    memset(searchName, 0x00, kFileNameSize);
                                    sd_read(searchName, kFileNameSize);
                                    if (strcmp(sampleName[i], searchName) == 0) {
                                        sampleNum[i] = searchCounter;
                                        sampleMissing[i] = false;
                                        searchActive = false;
                                    } else if (searchCounter >= sampleLibrarySize[instNum[i]]) {
                                        sampleNum[i] = -1;
                                        sampleMissing[i] = true;
                                        searchActive = false;
                                    }
                                    searchCounter += 1;
                                }
                            } else {
                                sampleMissing[i] = true;
                            }
                            f_close(&sd.file);
                        } else {
                            sampleNum[i] = -1;
                            sampleMissing[i] = false;
                        }

                        // Apply sample selection state to layer
                        if (sampleMissing[i]) {
                            layerInst_setInstSelected(i, -1);
                            layerInst_setInstLoaded(i);
                            layerInst_setSampleSelected(i, -1);
                            layerInst_setSampleLoaded(i);
                        } else {
                            layerInst_setSampleSelected(i, sampleNum[i]);
                            layerInst_setSampleLoaded(i);
                        }
                    }
                    phaseCheck[0] = true;
                }
            }
        }
        f_close(&sd.file);
        layerSong_selectBank(0);

        // 6. PHASE-2 LOAD: SONG PATTERN DATA
        // Read each layer's bank/beat sequence and rebuild arranged beat state.
        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            if (f_lseek(&sd.file, 1525) == FR_OK) {
                const uint16_t bufferByteSize = 512;
                const uint16_t readByteSize = kSongByteSize;
                uint16_t readCountTotal = (readByteSize / bufferByteSize);
                uint16_t remainderByteSize = (readByteSize % bufferByteSize);
                uint32_t errorCount = 0;

                // 6.1 LAYER LOOP
                // Read and decode song block for each layer separately.
                for (uint16_t i = 0; i < kLayerLibrarySize; i++) {
                    memset(data, 0x00, sizeof(data));
                    uint8_t dataRead[bufferByteSize] = {0};
                    uint32_t pointerOffset = 0;

                    // 6.2 BLOCK READ
                    // Read fixed-size blocks to assemble one layer payload.
                    for (uint16_t i = 0; i < readCountTotal; i++) {
                        if (sd_read(&dataRead, bufferByteSize) != FR_OK) errorCount += 1;
                        for (uint16_t j = 0; j < bufferByteSize; j++) {
                            volatile uint8_t* writePtr = (volatile uint8_t*)(data + pointerOffset);
                            *writePtr = dataRead[j];
                            pointerOffset += 1;
                        }
                        memset(dataRead, 0x00, bufferByteSize);
                    }

                    // 6.3 TAIL READ
                    // Read the final partial block when needed.
                    if (remainderByteSize) {
                        if (sd_read(&dataRead, remainderByteSize) != FR_OK) errorCount += 1;
                        for (uint16_t k = 0; k < remainderByteSize; k++) {
                            volatile uint8_t* writePtr = (volatile uint8_t*)(data + pointerOffset);
                            *writePtr = dataRead[k];
                            pointerOffset += 1;
                        }
                    }

                    // 6.4 BEAT DECODE
                    // Decode packed beat metadata and restore bank beat lists.
                    Layer& layer = layerLibrary[i];
                    for (uint8_t j = 0; j < kBankLibrarySize; j++) {
                        Bank& bank = layer.bankLibrary[j];
                        uint16_t offsetBank = 1 + (j * 257);

                        // Decode beat entries in reverse index order
                        for (int8_t k = (kBeatLibrarySize - 1); k >= 0; k--) {
                            uint16_t offsetBeat = offsetBank + 1 + (k * 4);
                            uint8_t dataBeat0 = data[offsetBeat + 0];
                            uint8_t dataBeat1 = data[offsetBeat + 1];
                            uint16_t dataBeat = ((uint16_t)dataBeat0 << 8) + dataBeat1;
                            bool active = (dataBeat >> 15);

                            // Extract packed beat fields only for active beats
                            if (active) {
                                uint8_t fillType = static_cast<uint8_t>((dataBeat & Beat::TYPE_MASK) >> Beat::TYPE_SHIFT);
                                uint8_t fillPattern = static_cast<uint8_t>((dataBeat & Beat::PATTERN_MASK) >> Beat::PATTERN_SHIFT);
                                uint8_t fillTime = static_cast<uint8_t>((dataBeat & Beat::TIME_MASK) >> Beat::TIME_SHIFT);
                                uint8_t fillLevel = static_cast<uint8_t>(dataBeat & Beat::LEVEL_MASK);
                                uint8_t startInterval0 = data[offsetBeat + 2];
                                uint8_t startInterval1 = data[offsetBeat + 3];
                                uint16_t startInterval = ((uint16_t)startInterval0 << 8) + startInterval1;
                                layerSong_setBeat(i, j, startInterval, fillType, fillPattern, fillTime, fillLevel, true);
                            }
                        }

                        // Normalize/arrange active beat ordering for runtime playback
                        layerSong_arrangeActiveBeats(i, j, true, true, true, true);
                    }
                }
                if (errorCount == 0) phaseCheck[1] = true;
            }
        }
        f_close(&sd.file);

        // 6.5 TONE SETTINGS + NOTES (.rwn next to the project; absent = plain DRUM layers)
        sd_loadNotes(fileNum_);

        // 7. POST-LOAD VALIDATION
        // Raise alert if any instrument/sample reference cannot be resolved.
        uint8_t missingSample = 0;
        for (uint8_t j = 0; j < kLayerLibrarySize; j++) {
            if (instMissing[j]) missingSample += 1;
            if (sampleMissing[j]) missingSample += 1;
        }
        if (missingSample) {
            (missingSample == 1) ? alertType = ALERT_MISSINGSAMPLE : alertType = ALERT_MISSINGSAMPLES;
            lcd_drawAlert();
            HAL_Delay(1000);
            result = SD_ERROR;
        } else if ((phaseCheck[0]) && (phaseCheck[1])) {
            result = SD_OK;
        }
        f_close(&sd.file);
    }

    // 8. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return result;
}

SdResult Controller::sd_saveFile(uint8_t fileNum_) {
    /// @brief Serializes current project state and writes it to the selected SD slot.
    /// Encodes drumkit parameters, asset references, song data, and final EOF marker.

    SdResult result = SD_ERROR;
    char eof[] = "EOF";
    char fileName[50];
    char noteName[50];

    // 1. FILENAME CONSTRUCTION
    // File_NNN.rwf / .rwn for a slot, Autosave.rwf / .rwn for kAutosaveFileNum.
    projectFileName(fileNum_, fileName, sizeof(fileName));
    noteFileName(fileNum_, noteName, sizeof(noteName));
    uint8_t data[kDrumkitByteSize] = {0x00};

    // 2. PAYLOAD ENCODING
    // Build the drumkit payload buffer before starting SD write operations.

    // 2.1 SLOT MARKER
    // Mark file as active project slot.
    data[0] = '*';

    // 2.2 MIXER SETTINGS
    data[1] = mixer.masterVolume;
    data[2] = mixer.hpVolume;
    data[3] = mixer.loVolume;
    data[4] = mixer.liVolume;
    data[5] = mixer.liMixerIn;
    data[6] = mixer.pan;
    data[7] = mixer.limiter;
    data[8] = mixer.normalize;
    data[9] = mixer.masterMute;
    data[10] = mixer.hpMute;
    data[11] = mixer.loMute;
    data[12] = mixer.liMute;

    // 2.3 RHYTHM SETTINGS
    data[13] = rhythm.tempo;
    data[14] = rhythm.measure;
    data[15] = rhythm.bar;
    data[16] = rhythm.quantize;

    // 2.4 METRONOME SETTINGS
    data[17] = metronome.active;
    data[18] = metronome.precount;
    data[19] = metronome.sample;
    data[20] = metronome.volume;

    // 2.5 I/O SYNC SETTINGS
    data[21] = io.syncIn;
    data[22] = io.syncOut;
    data[23] = (uint8_t)((io.midiIn << 4) | io.midiInChannel);
    data[24] = (uint8_t)((io.midiOut << 4) | io.midiOutChannel);

    // 2.6 EQUALIZER SETTINGS
    data[25] = eq.active;
    data[26] = eq.q;
    data[27] = eq.freqLowShelf;
    data[28] = eq.gainLowShelf;
    data[29] = eq.freqHighShelf;
    data[30] = eq.gainHighShelf;
    data[31] = eq.freqPeak[0];
    data[32] = eq.freqPeak[1];
    data[33] = eq.freqPeak[2];
    data[34] = eq.freqPeak[3];
    data[35] = eq.gainPeak[0];
    data[36] = eq.gainPeak[1];
    data[37] = eq.gainPeak[2];
    data[38] = eq.gainPeak[3];

    // 2.7 FILTER SETTINGS (0 AND 1)
    data[39] = filter[0].active;
    data[40] = filter[0].type;
    data[41] = filter[0].frequency;
    data[42] = filter[0].resonance;
    data[43] = filter[0].slope;
    data[44] = filter[0].drive;
    data[45] = filter[0].dry;
    data[46] = filter[0].wet;

    data[47] = filter[1].active;
    data[48] = filter[1].type;
    data[49] = filter[1].frequency;
    data[50] = filter[1].resonance;
    data[51] = filter[1].slope;
    data[52] = filter[1].drive;
    data[53] = filter[1].dry;
    data[54] = filter[1].wet;

    // 2.8 EFFECT SETTINGS (A/B + SUBEFFECTS)
    data[55] = effect[0].active;
    data[56] = effect[0].type;
    data[57] = effect[1].active;
    data[58] = effect[1].type;
    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
            uint16_t baseNum = 59 + (54 * i) + (6 * j);
            data[baseNum + 0] = effect[i].subEffect[j].aData;
            data[baseNum + 1] = effect[i].subEffect[j].bData;
            data[baseNum + 2] = effect[i].subEffect[j].cData;
            data[baseNum + 3] = effect[i].subEffect[j].dData;
            data[baseNum + 4] = effect[i].subEffect[j].eData;
            data[baseNum + 5] = effect[i].subEffect[j].fData;
        }
    }

    // 2.9 REVERB SETTINGS
    data[167] = reverb.active;
    data[168] = reverb.size;
    data[169] = reverb.decay;
    data[170] = reverb.damping;
    data[171] = reverb.preDelay;
    data[172] = reverb.surround;
    data[173] = reverb.dry;
    data[174] = reverb.wet;

    // 2.10 LFO SETTINGS
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        Lfo& lfo = lfoLibrary[i];
        uint16_t offset = 180 + (i * 6);
        data[offset + 0] = lfo.active;
        data[offset + 1] = lfo.type;
        data[offset + 2] = lfo.rate;
        data[offset + 3] = lfo.phase;
        data[offset + 4] = lfo.invert;
        data[offset + 5] = lfo.depth;
    }

    // 2.11 LAYER SETTINGS
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        Layer& layer = layerLibrary[i];
        uint16_t offset = 240 + (i * 25);
        data[offset + 0] = layer.level;
        data[offset + 1] = layer.pitch;
        data[offset + 2] = layer.direction;
        data[offset + 3] = layer.resolution;
        data[offset + 4] = layer.start;
        data[offset + 5] = layer.end;
        data[offset + 6] = layer.timing;
        data[offset + 7] = layer.probability;
        data[offset + 8] = layer.lfoLevel;
        data[offset + 9] = layer.lfoPitch;
        data[offset + 10] = layer.lfoDirection;
        data[offset + 11] = layer.lfoResolution;
        data[offset + 12] = layer.lfoStart;
        data[offset + 13] = layer.lfoEnd;
        data[offset + 14] = layer.lfoTiming;
        data[offset + 15] = layer.lfoProbability;
        data[offset + 16] = layer.eq;
        data[offset + 17] = layer.filter;
        data[offset + 18] = layer.effect;
        data[offset + 19] = layer.reverb;
        data[offset + 20] = layer.mute;
        data[offset + 21] = layer.fill;
        data[offset + 22] = layer.style;
    }

    // 2.12 ASSET REFERENCES
    // Store per-layer instrument/sample activity flags and names.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        Layer& layer = layerLibrary[i];
        uint16_t offset = 500 + (i * 100);
        if (layer.instLoaded != -1) {
            data[offset] = 'x';
            for (uint8_t j = 0; j < kFileNameSize; j++) {
                data[offset + 1 + j] = layer.instLoadedData.nameLong[j];
            }
        } else {
            data[offset] = 'o';
        }
        if (layer.sampleLoaded != -1) {
            data[offset + 50] = 'x';
            for (uint8_t j = 0; j < kFileNameSize; j++) {
                data[offset + 50 + 1 + j] = layer.sampleLoadedData.nameLong[j];
            }
        } else {
            data[offset + 50] = 'o';
        }
    }

    // 3. INPUT PROTECTION
    // Disable keyboard interrupts during SD write operations.
    keyboard_disable();

    // 4. SD STATE RECOVERY
    // Close stale handles and abort unfinished SD transactions.
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
        HAL_SD_Abort(&hsd2);
    }

    // SONG MODE CHAIN (v1.07+): project data only (not drumkits), stored in bytes of the
    // drumkit-style block that no other field uses (see kSongChainHeaderOffset in Global.h).
    {
        uint8_t chainBytes[songchain::kEncodedSize];
        songchain::encode(songChain, chainBytes);
        data[kSongChainHeaderOffset] = chainBytes[0];
        for (uint8_t i = 0; i < songchain::kMaxSteps; i++) data[kSongChainStepsOffset + i] = chainBytes[1 + i];
        songchain::encodeTranspose(songChain, &data[kSongTransposeOffset]);  // v2.03
    }

    // v2.02: the project's name.
    pname::encode(projectName, &data[pname::kNameOffset]);

    // AUTOSAVE (v2.01): the auto-save remembers which slot the session belongs to.
    if (fileNum_ == kAutosaveFileNum) {
        data[kAutosaveOriginOffset] = 'S';
        data[kAutosaveOriginOffset + 1] = (currentFileSlot >= 0) ? static_cast<uint8_t>(currentFileSlot + 1) : 0;
    }

    // 5. FILE WRITE PIPELINE (crash-safe since v2.01)
    // Write header, drumkit block, song blocks, and EOF marker to a temporary file; the notes
    // go to a second one. Only when both are complete does the commit replace the slot's
    // files, so a power cut mid-save never destroys the previous version.
    sd_beginSafeSave();
    if (sd_checkFile(fileNum_) == SD_OK) {
        if (f_open(&sd.file, kSaveTempMain, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            if (sd_write(kFileRef, strlen(kFileRef)) == FR_OK) {
                // 5.1 DRUMKIT BLOCK WRITE
                // Stream encoded drumkit payload in 512-byte chunks.
                const uint16_t writeByteSize = 512;
                uint16_t writeCountTotal = (kDrumkitByteSize / writeByteSize);
                uint16_t remainderByteSize = (kDrumkitByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;
                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (sd_write((data + pointerOffset), writeByteSize) != FR_OK) errorCount += 1;
                    pointerOffset += 512;
                }

                // 5.2 DRUMKIT TAIL WRITE
                // Write final non-512-aligned remainder.
                if (remainderByteSize) {
                    if (sd_write((data + pointerOffset), remainderByteSize) != FR_OK) errorCount += 1;
                }

                // 5.3 SONG ENCODE + WRITE
                // Encode each layer's banks/beats and write serialized song payload.
                for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
                    memset(data, 0x00, sizeof(data));
                    data[0] = 'l';
                    Layer& layer = layerLibrary[l];

                    // Encode all banks for current layer
                    for (uint8_t b = 0; b < kBankLibrarySize; b++) {
                        Bank& bank = layer.bankLibrary[b];
                        uint16_t offsetBank = 1 + (b * 257);
                        data[offsetBank] = 'b';

                        // Encode packed beat data + start interval
                        for (uint8_t k = 0; k < kBeatLibrarySize; k++) {
                            Beat& beat = bank.beatLibrary[k];
                            uint16_t offsetBeat = offsetBank + 1 + (k * 4);
                            uint16_t data_ = beat.getData();
                            data[offsetBeat + 0] = (uint8_t)(data_ >> 8);
                            data[offsetBeat + 1] = (uint8_t)(data_ & 0xFF);
                            uint16_t startInterval = beat.getStartInterval();
                            data[offsetBeat + 2] = (uint8_t)(startInterval >> 8);
                            data[offsetBeat + 3] = (uint8_t)(startInterval & 0xFF);
                        }
                    }

                    // Write song payload in 512-byte chunks
                    writeCountTotal = (kSongByteSize / writeByteSize);
                    remainderByteSize = (kSongByteSize % writeByteSize);
                    pointerOffset = 0;
                    for (uint16_t i = 0; i < writeCountTotal; i++) {
                        if (sd_write((data + pointerOffset), writeByteSize) != FR_OK) errorCount += 1;
                        pointerOffset += 512;
                    }

                    // Write song tail remainder
                    if (remainderByteSize) {
                        if (sd_write((data + pointerOffset), remainderByteSize) != FR_OK) errorCount += 1;
                    }
                }

                // 5.4 EOF COMMIT
                // Commit EOF marker only when all previous writes succeeded.
                if (errorCount == 0) {
                    if (sd_write(eof, strlen(eof)) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        // f_close flushes the last cached sector; a failure here means the file is incomplete.
        if (f_close(&sd.file) != FR_OK) result = SD_ERROR;

        // TONE settings and notes go to the .rwn file next to it; losing them is a save error.
        bool notesWritten = false;
        if ((result == SD_OK) && (sd_saveNotes(kSaveTempSide, notesWritten) != SD_OK)) result = SD_ERROR;

        // Commit: a drum-only project has no .rwn (an old one is removed).
        if (result == SD_OK) {
            result = sd_commitSafeSave(fileName, noteName, notesWritten ? autosave::kSideReplace : autosave::kSideDelete);
        }
    }
    if (result != SD_OK) sd_beginSafeSave();  // throw away whatever a failed save left behind

    // 6. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return result;
}

SdResult Controller::sd_clearFile(uint8_t fileNum_) {
    /// @brief Overwrites a project file slot with default factory data.
    /// Resets drumkit configurations and song patterns to an empty state.

    SdResult result = SD_ERROR;
    char eof[] = "EOF";
    char fileName[50];
    char noteName[50];

    // 1. FILENAME CONSTRUCTION
    // Build the target paths (e.g., System/File/File_001.rwf / .rwn).
    projectFileName(fileNum_, fileName, sizeof(fileName));
    noteFileName(fileNum_, noteName, sizeof(noteName));

    // 2. INPUT PROTECTION + SD STATE RECOVERY
    // Lock keyboard input and ensure SDMMC is in a clean 'Ready' state.
    keyboard_disable();
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
        HAL_SD_Abort(&hsd2);
    }

    // 3. FILE RE-CREATION (crash-safe since v2.01: temporary file + commit)
    // Re-create the target slot to wipe any previous project data.
    sd_beginSafeSave();
    if (sd_checkFile(fileNum_) == SD_OK) {
        if (f_open(&sd.file, kSaveTempMain, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            // 4. HEADER WRITE
            // Write standard project signature header.
            if (sd_write(kFileRef, strlen(kFileRef)) == FR_OK) {
                const uint16_t writeByteSize = 512;  // Chunk size for SD block writing
                uint16_t writeCountTotal;
                uint16_t remainderByteSize;
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                // 5. DEFAULT PAYLOAD WRITE
                // Stream default drumkit/song templates in fixed-size blocks.

                // 5.1 DRUMKIT BLOCK WRITE
                // Write default drumkit data in 512-byte chunks.
                writeCountTotal = (kDrumkitByteSize / writeByteSize);
                remainderByteSize = (kDrumkitByteSize % writeByteSize);
                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (sd_write((kDataDrumkit + pointerOffset), writeByteSize) != FR_OK) errorCount += 1;
                    pointerOffset += 512;
                }

                // 5.2 DRUMKIT TAIL WRITE
                // Write final non-512-aligned drumkit remainder.
                if (remainderByteSize) {
                    if (sd_write((kDataDrumkit + pointerOffset), remainderByteSize) != FR_OK) errorCount += 1;
                }

                // 5.3 SONG BLOCK WRITE
                // Iterate all layers and write default sequencer templates.
                writeCountTotal = (kSongByteSize / writeByteSize);
                remainderByteSize = (kSongByteSize % writeByteSize);
                for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
                    pointerOffset = 0;
                    for (uint16_t i = 0; i < writeCountTotal; i++) {
                        if (sd_write((kDataSong + pointerOffset), writeByteSize) != FR_OK) errorCount += 1;
                        pointerOffset += 512;
                    }

                    // 5.4 SONG TAIL WRITE
                    // Write final non-512-aligned song remainder.
                    if (remainderByteSize) {
                        if (sd_write((kDataSong + pointerOffset), remainderByteSize) != FR_OK) errorCount += 1;
                    }
                }

                // 5.5 EOF COMMIT
                // Seal the file with EOF only if all previous writes succeeded.
                if (errorCount == 0) {
                    if (sd_write(eof, strlen(eof)) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        // f_close flushes the last cached sector; a failure here means the file is incomplete.
        if (f_close(&sd.file) != FR_OK) result = SD_ERROR;

        // An empty slot has no notes either.
        if (result == SD_OK) result = sd_commitSafeSave(fileName, noteName, autosave::kSideDelete);
    }
    if (result != SD_OK) sd_beginSafeSave();

    // 6. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return result;
}

/* Note Files (TONE layers) ---------------------------------------------------*/

// System/File/File_NNN.rwn, next to File_NNN.rwf:
//   "RWNOTES1"
//   10 x [mode, root, scale, glide]
//   10 layers x 5 banks x { count, count x [start interval hi, lo, note] }  (only notes != root)
//   "EOF"
//   "SYN5" + 10 x [the 20 values of synth::pack: sound, LFO, AMP, filter type] + "EOF"   (v1.13;
//          "SYN2" from v1.12 has the first 10 values, "SYN1" from v1.11 the first 7; both still
//          read, as are the v1.13 test builds' "SYN3"/"SYN4")
//   "SLD1" + 10 layers x 5 banks x { count, count x [start interval hi, lo] } + "EOF"   (v1.12)
//   "HLD1" + 10 layers x 5 banks x { count, count x [start hi, lo, hold length hi, lo] } + "EOF"
//          (v1.13: notes recorded with PAD HOLD; older readers stop before it)
//   "HRM1" + 10 x [follow (0 = off, 1..10 = layer), interval] + "EOF"   (v2.04: harmony layers)
// Mode 2 (SYNTH) is read as DRUM by v1.09/v1.10, which stop at the first "EOF".
// Notes are matched to beats by start interval on load (positions are unique within a bank),
// so the file never depends on slot order. Older firmware never opens it.

void Controller::noteFileName(uint8_t fileNum_, char* name_, size_t size_) {
    if (fileNum_ == kAutosaveFileNum) {
        snprintf(name_, size_, "%s%s", kAutosaveFileBase, kNoteFileEnd);
    } else {
        snprintf(name_, size_, "%s%03u%s", kFileStart, static_cast<unsigned>(fileNum_ + 1), kNoteFileEnd);
    }
}

void Controller::projectFileName(uint8_t fileNum_, char* name_, size_t size_) {
    if (fileNum_ == kAutosaveFileNum) {
        snprintf(name_, size_, "%s%s", kAutosaveFileBase, kFileEnd);
    } else {
        snprintf(name_, size_, "%s%03u%s", kFileStart, static_cast<unsigned>(fileNum_ + 1), kFileEnd);
    }
}

SdResult Controller::sd_saveNotes(const char* path_, bool& written_) {
    /// @brief Writes the TONE settings and notes of the project to path_ (the temporary side
    /// file of a safe save). A project that uses neither gets no note file (written_ = false;
    /// the commit then removes a stale one), so drum-only projects look exactly as before.

    written_ = false;

    bool needed = false;
    for (uint8_t l = 0; (l < kLayerLibrarySize) && !needed; l++) {
        const Layer& layer = layerLibrary[l];
        if ((layer.toneMode != tone::kModeDrum) || (layer.toneRoot != 0) || (layer.toneScale != tone::kDefaultScale) || (layer.toneGlide != 0)) needed = true;
        if (!synth::isDefault(layer.synth)) needed = true;
        if (layer.followLayer >= 0) needed = true;  // v2.04 harmony
        if (!arp::isDefault(layer.arp)) needed = true;  // v2.08 arpeggiator
        for (uint8_t b = 0; (b < kBankLibrarySize) && !needed; b++) {
            const Bank& bank = layer.bankLibrary[b];
            for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
                if (bank.beatLibrary[k].getActive() && ((bank.beatLibrary[k].getNote() != 0) || bank.beatLibrary[k].getSlide() || (bank.beatLibrary[k].getHold() != 0))) {
                    needed = true;
                    break;
                }
            }
        }
    }

    if (!needed) return SD_OK;

    if (f_open(&sd.file, path_, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return SD_ERROR;
    written_ = true;
    uint32_t errorCount = 0;

    if (sd_write(kNoteFileRef, strlen(kNoteFileRef)) != FR_OK) errorCount += 1;

    uint8_t settings[kLayerLibrarySize * 4];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        const Layer& layer = layerLibrary[l];
        settings[(l * 4) + 0] = layer.toneMode;
        settings[(l * 4) + 1] = layer.toneRoot;
        settings[(l * 4) + 2] = layer.toneScale;
        settings[(l * 4) + 3] = layer.toneGlide;
    }
    if (sd_write(settings, sizeof(settings)) != FR_OK) errorCount += 1;

    uint8_t record[1 + (kBeatLibrarySize * 3)];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        for (uint8_t b = 0; b < kBankLibrarySize; b++) {
            const Bank& bank = layerLibrary[l].bankLibrary[b];
            uint8_t count = 0;
            for (int16_t k = 0; (k <= bank.lastActiveBeatNum) && (count < kBeatLibrarySize); k++) {
                const Beat& beat = bank.beatLibrary[k];
                if (!beat.getActive() || (beat.getNote() == 0)) continue;
                const uint16_t start = beat.getStartInterval();
                record[1 + (count * 3) + 0] = static_cast<uint8_t>(start >> 8);
                record[1 + (count * 3) + 1] = static_cast<uint8_t>(start & 0xFF);
                record[1 + (count * 3) + 2] = static_cast<uint8_t>(beat.getNote());
                count++;
            }
            record[0] = count;
            if (sd_write(record, 1 + (count * 3)) != FR_OK) errorCount += 1;
        }
    }

    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    // v1.11+: the SYNTH sound of every layer, after the part v1.09/v1.10 read (they stop at
    // "EOF" and see a SYNTH layer as a DRUM layer with its notes intact).
    uint8_t sound[kLayerLibrarySize * kSynthFileParams];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) synth::pack(layerLibrary[l].synth, &sound[l * kSynthFileParams]);
    if (sd_write(kSynthFileRef, strlen(kSynthFileRef)) != FR_OK) errorCount += 1;
    if (sd_write(sound, sizeof(sound)) != FR_OK) errorCount += 1;
    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    // v1.12+: beats with SLIDE, as start intervals per layer and bank.
    if (sd_write(kSlideFileRef, strlen(kSlideFileRef)) != FR_OK) errorCount += 1;
    uint8_t slides[1 + (kBeatLibrarySize * 2)];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        for (uint8_t b = 0; b < kBankLibrarySize; b++) {
            const Bank& bank = layerLibrary[l].bankLibrary[b];
            uint8_t count = 0;
            for (int16_t k = 0; (k <= bank.lastActiveBeatNum) && (count < kBeatLibrarySize); k++) {
                const Beat& beat = bank.beatLibrary[k];
                if (!beat.getActive() || !beat.getSlide()) continue;
                slides[1 + (count * 2) + 0] = static_cast<uint8_t>(beat.getStartInterval() >> 8);
                slides[1 + (count * 2) + 1] = static_cast<uint8_t>(beat.getStartInterval() & 0xFF);
                count++;
            }
            slides[0] = count;
            if (sd_write(slides, 1 + (count * 2)) != FR_OK) errorCount += 1;
        }
    }
    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    // v1.13+: notes recorded with PAD HOLD, as start interval + length per layer and bank.
    if (sd_write(kHoldFileRef, strlen(kHoldFileRef)) != FR_OK) errorCount += 1;
    uint8_t holds[1 + (kBeatLibrarySize * 4)];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        for (uint8_t b = 0; b < kBankLibrarySize; b++) {
            const Bank& bank = layerLibrary[l].bankLibrary[b];
            uint8_t count = 0;
            for (int16_t k = 0; (k <= bank.lastActiveBeatNum) && (count < kBeatLibrarySize); k++) {
                const Beat& beat = bank.beatLibrary[k];
                if (!beat.getActive() || (beat.getHold() == 0)) continue;
                holds[1 + (count * 4) + 0] = static_cast<uint8_t>(beat.getStartInterval() >> 8);
                holds[1 + (count * 4) + 1] = static_cast<uint8_t>(beat.getStartInterval() & 0xFF);
                holds[1 + (count * 4) + 2] = static_cast<uint8_t>(beat.getHold() >> 8);
                holds[1 + (count * 4) + 3] = static_cast<uint8_t>(beat.getHold() & 0xFF);
                count++;
            }
            holds[0] = count;
            if (sd_write(holds, 1 + (count * 4)) != FR_OK) errorCount += 1;
        }
    }
    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    // v2.04: harmony layers, FOLLOW + interval per layer (older readers stop before it).
    if (sd_write(kHarmonyFileRef, strlen(kHarmonyFileRef)) != FR_OK) errorCount += 1;
    uint8_t follow[kLayerLibrarySize * 2];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        follow[(l * 2) + 0] = harmony::encodeFollow(layerLibrary[l].followLayer);
        follow[(l * 2) + 1] = harmony::safeInterval(layerLibrary[l].harmonyInterval);
    }
    if (sd_write(follow, sizeof(follow)) != FR_OK) errorCount += 1;
    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    // v2.05: the synth MOD page (FM, pitch envelope, filter attack) of every layer.
    if (sd_write(kSynthExtRef, strlen(kSynthExtRef)) != FR_OK) errorCount += 1;
    uint8_t ext[kLayerLibrarySize * synth::kExtSize];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) synth::packExt(layerLibrary[l].synth, &ext[l * synth::kExtSize]);
    if (sd_write(ext, sizeof(ext)) != FR_OK) errorCount += 1;
    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    // v2.08: the ARP page of every layer.
    if (sd_write(kArpRef, strlen(kArpRef)) != FR_OK) errorCount += 1;
    uint8_t arps[kLayerLibrarySize * arp::kPackedSize];
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) arp::pack(layerLibrary[l].arp, &arps[l * arp::kPackedSize]);
    if (sd_write(arps, sizeof(arps)) != FR_OK) errorCount += 1;
    if (sd_write("EOF", 3) != FR_OK) errorCount += 1;

    if (f_close(&sd.file) != FR_OK) errorCount += 1;
    return (errorCount == 0) ? SD_OK : SD_ERROR;
}

void Controller::sd_loadNotes(uint8_t fileNum_) {
    /// @brief Restores TONE settings and notes after the project's beats are loaded. No file
    /// (drum-only project, or one saved by older firmware) means plain DRUM layers.

    tone_resetAll();

    char name[50];
    noteFileName(fileNum_, name, sizeof(name));
    if (f_open(&sd.file, name, FA_READ) != FR_OK) return;

    char ref[sizeof(kNoteFileRef)] = {};
    uint8_t settings[kLayerLibrarySize * 4];
    if ((sd_read(ref, strlen(kNoteFileRef)) == FR_OK) && (strncmp(ref, kNoteFileRef, strlen(kNoteFileRef)) == 0) &&
        (sd_read(settings, sizeof(settings)) == FR_OK)) {
        // Settings, range-checked so a damaged file can never index outside a table.
        for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
            Layer& layer = layerLibrary[l];
            layer.toneMode = (settings[(l * 4) + 0] < tone::kModeCount) ? settings[(l * 4) + 0] : tone::kModeDrum;
            layer.toneRoot = (settings[(l * 4) + 1] < tone::kRootCount) ? settings[(l * 4) + 1] : 0;
            layer.toneScale = tone::safeScale(settings[(l * 4) + 2]);
            tone_setGlide(l, settings[(l * 4) + 3]);
        }

        // Notes, matched to beats by position.
        bool ok = true;
        for (uint8_t l = 0; (l < kLayerLibrarySize) && ok; l++) {
            for (uint8_t b = 0; (b < kBankLibrarySize) && ok; b++) {
                Bank& bank = layerLibrary[l].bankLibrary[b];
                uint8_t count = 0;
                if ((sd_read(&count, 1) != FR_OK) || (count > kBeatLibrarySize)) {
                    ok = false;
                    break;
                }
                for (uint8_t e = 0; e < count; e++) {
                    uint8_t entry[3];
                    if (sd_read(entry, 3) != FR_OK) {
                        ok = false;
                        break;
                    }
                    const uint16_t start = static_cast<uint16_t>((entry[0] << 8) | entry[1]);
                    const int8_t note = tone::clampDegree(static_cast<int8_t>(entry[2]), 0);  // widest range (chromatic)
                    for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
                        Beat& beat = bank.beatLibrary[k];
                        if (beat.getActive() && (beat.getStartInterval() == start)) {
                            beat.setNote(note);
                            break;
                        }
                    }
                }
            }
        }

        // SYNTH sounds after the notes' "EOF": "SYN4" (v1.13, 15 values with the LFO), "SYN2"
        // (v1.12, 10) or "SYN1" (v1.11, 7). "SYN3" was a v1.13 test build (11; its 11th value, a
        // global-LFO slot, is ignored). Files from v1.09/v1.10 end there: default sounds.
        char eof[3];
        char tag[5] = {};
        uint8_t sound[kLayerLibrarySize * kSynthFileParams];
        uint8_t perLayer = 0;
        uint8_t used = 0;
        if (ok && (sd_read(eof, 3) == FR_OK) && (sd_read(tag, 4) == FR_OK)) {
            if (strncmp(tag, "SYN6", 4) == 0) perLayer = used = 21;
            if (strncmp(tag, "SYN5", 4) == 0) perLayer = used = 20;
            if (strncmp(tag, "SYN4", 4) == 0) perLayer = used = 15;
            if (strncmp(tag, "SYN3", 4) == 0) { perLayer = 11; used = 10; }
            if (strncmp(tag, "SYN2", 4) == 0) perLayer = used = 10;
            if (strncmp(tag, "SYN1", 4) == 0) perLayer = used = 7;
        }
        static_assert(kSynthFileParams == 21, "SYN6 holds 21 values per layer");
        if ((perLayer > 0) && (sd_read(sound, kLayerLibrarySize * perLayer) == FR_OK)) {
            for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
                layerLibrary[l].synth = synth::unpack(&sound[l * perLayer], used);  // sanitized
            }

            // Beats with SLIDE (v1.12+), after the sounds' "EOF".
            char slideTag[sizeof(kSlideFileRef)] = {};
            if ((sd_read(eof, 3) == FR_OK) && (sd_read(slideTag, strlen(kSlideFileRef)) == FR_OK) &&
                (strncmp(slideTag, kSlideFileRef, strlen(kSlideFileRef)) == 0)) {
                bool slidesOk = true;
                for (uint8_t l = 0; (l < kLayerLibrarySize) && slidesOk; l++) {
                    for (uint8_t b = 0; (b < kBankLibrarySize) && slidesOk; b++) {
                        Bank& bank = layerLibrary[l].bankLibrary[b];
                        uint8_t count = 0;
                        if ((sd_read(&count, 1) != FR_OK) || (count > kBeatLibrarySize)) {
                            slidesOk = false;
                            break;
                        }
                        for (uint8_t e = 0; e < count; e++) {
                            uint8_t entry[2];
                            if (sd_read(entry, 2) != FR_OK) {
                                slidesOk = false;
                                break;
                            }
                            const uint16_t start = static_cast<uint16_t>((entry[0] << 8) | entry[1]);
                            for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
                                if (bank.beatLibrary[k].getActive() && (bank.beatLibrary[k].getStartInterval() == start)) {
                                    bank.beatLibrary[k].setSlide(true);
                                    break;
                                }
                            }
                        }
                    }
                }

                // Notes recorded with PAD HOLD (v1.13+), after the slides' "EOF".
                char holdTag[sizeof(kHoldFileRef)] = {};
                if (slidesOk && (sd_read(eof, 3) == FR_OK) && (sd_read(holdTag, strlen(kHoldFileRef)) == FR_OK) &&
                    (strncmp(holdTag, kHoldFileRef, strlen(kHoldFileRef)) == 0)) {
                    bool holdsOk = true;
                    for (uint8_t l = 0; (l < kLayerLibrarySize) && holdsOk; l++) {
                        for (uint8_t b = 0; (b < kBankLibrarySize) && holdsOk; b++) {
                            Bank& bank = layerLibrary[l].bankLibrary[b];
                            uint8_t count = 0;
                            if ((sd_read(&count, 1) != FR_OK) || (count > kBeatLibrarySize)) {
                                holdsOk = false;
                                break;
                            }
                            for (uint8_t e = 0; e < count; e++) {
                                uint8_t entry[4];
                                if (sd_read(entry, 4) != FR_OK) {
                                    holdsOk = false;
                                    break;
                                }
                                const uint16_t start = static_cast<uint16_t>((entry[0] << 8) | entry[1]);
                                const uint16_t hold = static_cast<uint16_t>((entry[2] << 8) | entry[3]);
                                for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
                                    if (bank.beatLibrary[k].getActive() && (bank.beatLibrary[k].getStartInterval() == start)) {
                                        bank.beatLibrary[k].setHold(hold);
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    // Harmony layers (v2.04+), after the holds' "EOF".
                    char harmonyTag[sizeof(kHarmonyFileRef)] = {};
                    uint8_t follow[kLayerLibrarySize * 2];
                    if (holdsOk && (sd_read(eof, 3) == FR_OK) && (sd_read(harmonyTag, strlen(kHarmonyFileRef)) == FR_OK) &&
                        (strncmp(harmonyTag, kHarmonyFileRef, strlen(kHarmonyFileRef)) == 0) && (sd_read(follow, sizeof(follow)) == FR_OK)) {
                        for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
                            layerLibrary[l].followLayer = harmony::decodeFollow(follow[(l * 2) + 0], l, kLayerLibrarySize);
                            layerLibrary[l].harmonyInterval = harmony::safeInterval(follow[(l * 2) + 1]);
                        }

                        // Synth MOD page (v2.05+), after the harmony's "EOF".
                        char extTag[sizeof(kSynthExtRef)] = {};
                        uint8_t ext[kLayerLibrarySize * synth::kExtSize];
                        if ((sd_read(eof, 3) == FR_OK) && (sd_read(extTag, strlen(kSynthExtRef)) == FR_OK) &&
                            (strncmp(extTag, kSynthExtRef, strlen(kSynthExtRef)) == 0) && (sd_read(ext, sizeof(ext)) == FR_OK)) {
                            for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
                                layerLibrary[l].synth = synth::unpackExt(layerLibrary[l].synth, &ext[l * synth::kExtSize]);
                            }

                            // Arpeggiator (v2.08+), after the MOD page's "EOF".
                            char arpTag[sizeof(kArpRef)] = {};
                            uint8_t arps[kLayerLibrarySize * arp::kPackedSize];
                            if ((sd_read(eof, 3) == FR_OK) && (sd_read(arpTag, strlen(kArpRef)) == FR_OK) &&
                                (strncmp(arpTag, kArpRef, strlen(kArpRef)) == 0) && (sd_read(arps, sizeof(arps)) == FR_OK)) {
                                for (uint8_t l = 0; l < kLayerLibrarySize; l++) layerLibrary[l].arp = arp::unpack(&arps[l * arp::kPackedSize]);  // sanitized
                            }
                        }
                    }
                }
            }
        }
    }
    f_close(&sd.file);
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        synthPresetIndex[l] = -1;       // a loaded sound is named by what it is
        lcd_drawLayerTab_InstData(l);   // SYNTH layers show their sound's name
    }
}

SdResult Controller::sd_checkDrumkit(uint8_t kitNum_) {
    /// @brief Validates a drumkit file's existence and internal status before loading.
    /// Categorizes the slot to prevent loading empty or corrupt drumkit data.

    SdResult result = SD_ERROR;
    drumkitStatus = FILE_NONE;
    pname::clear(kitSlotName);
    char refRead[sizeof(kDrumkitRef)] = {};
    char data;

    // 1. FILENAME CONSTRUCTION
    // Construct the 3-digit padded filename (e.g., Drumkit_001.rws).
    char drumkitName[50];
    char drumkitNum[4];
    sprintf(drumkitNum, "%03d", (kitNum_ + 1));
    strcpy(drumkitName, kDrumkitStart);
    strcat(drumkitName, drumkitNum);
    strcat(drumkitName, kDrumkitEnd);

    // 2. FILE ACCESS
    // Attempt to open the file. If it fails, the file is physically missing.
    if (f_open(&sd.file, drumkitName, FA_READ) == FR_OK) {
        // 3. SIGNATURE VERIFICATION
        // Read the file header and compare it against the global kDrumkitRef string.
        if (sd_read(refRead, strlen(kDrumkitRef)) == FR_OK) {
            if (strncmp(kDrumkitRef, refRead, strlen(kDrumkitRef)) == 0) {
                // 4. OCCUPANCY CHECK
                // If the signature matches, read the status byte. '*' denotes a saved drumkit.
                sd_read(&data, 1);
                (data == '*') ? drumkitStatus = FILE_ACTIVE : drumkitStatus = FILE_INACTIVE;

                // v2.02: the slot's name for the Drumkit menu.
                if (drumkitStatus == FILE_ACTIVE) {
                    uint8_t nameBytes[pname::kLength];
                    if ((f_lseek(&sd.file, strlen(kDrumkitRef) + pname::kNameOffset) == FR_OK) && (sd_read(nameBytes, pname::kLength) == FR_OK)) {
                        pname::decode(nameBytes, kitSlotName);
                    }
                }
            } else {
                // 5. COMPATIBILITY HANDLING
                // The file exists but the header is unrecognized or from an older version.
                drumkitStatus = FILE_INCOMPATIBLE;
            }
        } else {
            drumkitStatus = FILE_INCOMPATIBLE;
        }
    } else {
        // 6. ERROR STATE ASSIGNMENT
        drumkitStatus = FILE_MISSING;
    }

    // 7. RESOURCE CLEANUP
    // Always close the file handle to free up FatFS resources for the next operation.
    f_close(&sd.file);
    result = SD_OK;
    return result;
}

SdResult Controller::sd_loadDrumkit(uint8_t kitNum_) {
    /// @brief Loads a drumkit file and reconstructs runtime state.
    /// Decodes processor/layer parameters and validates referenced assets.

    SdResult result = SD_ERROR;
    char drumkitName[50];
    char drumkitNum[4];

    // 1. FILENAME CONSTRUCTION
    // Generate the 3-digit padded filename (e.g., Drumkit_001.rws).
    sprintf(drumkitNum, "%03d", (kitNum_ + 1));
    strcpy(drumkitName, kDrumkitStart);
    strcat(drumkitName, drumkitNum);
    strcat(drumkitName, kDrumkitEnd);

    // 2. INPUT PROTECTION
    // Disable keyboard interrupts to avoid bus contention during SD file I/O.
    keyboard_disable();

    // 3. SD STATE RECOVERY
    // Ensure stale file handles are closed and abort unfinished SD transactions.
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
        HAL_SD_Abort(&hsd2);
    }

    // 4. FILE ELIGIBILITY CHECK
    // Load only if file exists and is marked as an active drumkit slot.
    if ((sd_checkDrumkit(kitNum_) == SD_OK) && (drumkitStatus == FILE_ACTIVE)) {
        char data[kDrumkitByteSize] = {0x00};
        if (f_open(&sd.file, drumkitName, FA_READ) == FR_OK) {
            if (f_lseek(&sd.file, strlen(kDrumkitRef)) == FR_OK) {
                const uint16_t readByteSize = 512;
                const uint16_t drumkitByteSize = kDrumkitByteSize;
                uint16_t readCountTotal = (drumkitByteSize / readByteSize);
                uint16_t remainderByteSize = (drumkitByteSize % readByteSize);
                uint8_t dataRead[readByteSize] = {0};
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                // 5. DRUMKIT DATA READ
                // Read and decode drumkit payload from storage.

                // 5.1 BLOCK READ
                // Read fixed-size 512-byte chunks for efficient SD throughput.
                for (uint16_t i = 0; i < readCountTotal; i++) {
                    if (sd_read(&dataRead, readByteSize) != FR_OK) errorCount += 1;
                    for (uint16_t j = 0; j < readByteSize; j++) {
                        volatile uint8_t* writePtr = (volatile uint8_t*)(data + pointerOffset);
                        *writePtr = dataRead[j];
                        pointerOffset += 1;
                    }
                    memset(dataRead, 0x00, readByteSize);
                }

                // 5.2 TAIL READ
                // Read the final partial block when payload is not aligned to 512 bytes.
                if (remainderByteSize) {
                    if (sd_read(&dataRead, remainderByteSize) != FR_OK) errorCount += 1;
                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                        volatile uint8_t* writePtr = (volatile uint8_t*)(data + pointerOffset);
                        *writePtr = dataRead[k];
                        pointerOffset += 1;
                    }
                }
                f_close(&sd.file);
                if (errorCount == 0) {
                    // 5.3 PARAMETER DECODE
                    // Decode and apply mixer/rhythm/FX/layer parameters from the buffer.

                    // v2.02: the drumkit's name (none in kits from older firmware).
                    pname::decode(reinterpret_cast<const uint8_t*>(&data[pname::kNameOffset]), kitName);

                    // Mixer configuration

                    // Rhythm configuration

                    // Metronome settings

                    // IO settings

                    // Equalizer settings
                    eq_setActive((bool)data[25]);
                    eq_setQ(data[26]);
                    eq_setFreqLowShelf(data[27]);
                    eq_setGainLowShelf(data[28]);
                    eq_setFreqHighShelf(data[29]);
                    eq_setGainHighShelf(data[30]);
                    eq_setFreqPeak(0, data[31]);
                    eq_setFreqPeak(1, data[32]);
                    eq_setFreqPeak(2, data[33]);
                    eq_setFreqPeak(3, data[34]);
                    eq_setGainPeak(0, data[35]);
                    eq_setGainPeak(1, data[36]);
                    eq_setGainPeak(2, data[37]);
                    eq_setGainPeak(3, data[38]);

                    // Filter configuration (0 and 1)
                    filter_setActive(0, (bool)data[39]);
                    filter_setType(0, data[40]);
                    filter_setFrequency(0, data[41]);
                    filter_setResonance(0, data[42]);
                    filter_setSlope(0, data[43]);
                    filter_setDrive(0, data[44]);
                    filter_setDry(0, data[45]);
                    filter_setWet(0, data[46]);

                    filter_setActive(1, (bool)data[47]);
                    filter_setType(1, data[48]);
                    filter_setFrequency(1, data[49]);
                    filter_setResonance(1, data[50]);
                    filter_setSlope(1, data[51]);
                    filter_setDrive(1, data[52]);
                    filter_setDry(1, data[53]);
                    filter_setWet(1, data[54]);

                    // Effect processor settings (A/B and subeffects)
                    effect_setActive(0, (bool)data[55]);
                    effect_setType(0, data[56]);
                    effect_setActive(1, (bool)data[57]);
                    effect_setType(1, data[58]);
                    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
                        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
                            uint16_t baseNum = 59 + (54 * i) + (6 * j);
                            effect_setAData(i, j, data[baseNum + 0]);
                            effect_setBData(i, j, data[baseNum + 1]);
                            effect_setCData(i, j, data[baseNum + 2]);
                            effect_setDData(i, j, data[baseNum + 3]);
                            effect_setEData(i, j, data[baseNum + 4]);
                            effect_setFData(i, j, data[baseNum + 5]);
                        }
                    }

                    // Reverb processor parameters
                    reverb_setActive(data[167]);
                    reverb_setSize(data[168]);
                    reverb_setDecay(data[169]);
                    reverb_setDamping(data[170]);
                    reverb_setPreDelay(data[171]);
                    reverb_setSurround(data[172]);
                    reverb_setDry(data[173]);
                    reverb_setWet(data[174]);

                    // LFO modulation settings
                    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
                        uint16_t offset = 180 + (i * 6);
                        lfo_setActive(i, (bool)data[offset + 0]);
                        lfo_setType(i, data[offset + 1]);
                        lfo_setRate(i, data[offset + 2]);
                        lfo_setPhase(i, data[offset + 3]);
                        lfo_setInvert(i, data[offset + 4]);
                        lfo_setDepth(i, data[offset + 5]);
                    }

                    // Layer instrument configuration
                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                        uint16_t offset = 240 + (i * 25);
                        layerInst_setLevel(i, data[offset + 0]);
                        layerInst_setPitch(i, data[offset + 1]);
                        layerInst_setDirection(i, data[offset + 2]);
                        layerInst_setResolution(i, data[offset + 3]);
                        layerInst_setStart(i, data[offset + 4]);
                        layerInst_setEnd(i, data[offset + 5]);
                        layerInst_setTiming(i, data[offset + 6]);
                        layerInst_setProbability(i, data[offset + 7]);
                        layerInst_setLfoLevel(i, data[offset + 8]);
                        layerInst_setLfoPitch(i, data[offset + 9]);
                        layerInst_setLfoDirection(i, data[offset + 10]);
                        layerInst_setLfoResolution(i, data[offset + 11]);
                        layerInst_setLfoStart(i, data[offset + 12]);
                        layerInst_setLfoEnd(i, data[offset + 13]);
                        layerInst_setLfoTiming(i, data[offset + 14]);
                        layerInst_setLfoProbability(i, data[offset + 15]);
                        layerInst_setEq(i, data[offset + 16]);
                        layerInst_setFilter(i, data[offset + 17]);
                        layerInst_setEffect(i, data[offset + 18]);
                        layerInst_setReverb(i, data[offset + 19]);
                        layerInst_setMute(i, (bool)data[offset + 20]);
                        layerInst_setFill(i, (bool)data[offset + 21]);
                        layerInst_setStyle(i, (bool)data[offset + 22]);
                    }

                    // MODE + SYNTH sounds (v1.13 kits only; older kits leave them as they are)
                    sd_loadDrumkitSynth(drumkitName);

                    // 5.4 ASSET REFERENCE EXTRACTION
                    // Extract instrument/sample names and activation markers per layer.
                    char instName[kLayerLibrarySize][kFileNameSize + 1];
                    uint16_t instNum[kLayerLibrarySize];
                    bool instActive[kLayerLibrarySize];
                    bool instMissing[kLayerLibrarySize] = {false};
                    char sampleName[kLayerLibrarySize][kFileNameSize + 1];
                    uint16_t sampleNum[kLayerLibrarySize];
                    bool sampleActive[kLayerLibrarySize];
                    bool sampleMissing[kLayerLibrarySize];
                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                        Layer& layer = layerLibrary[i];
                        uint16_t offset = 500 + (i * 100);
                        (data[offset] == 'x') ? instActive[i] = true : instActive[i] = false;
                        (data[offset + 50] == 'x') ? sampleActive[i] = true : sampleActive[i] = false;
                        memcpy(instName[i], &(data[offset + 1]), kFileNameSize + 1);
                        memcpy(sampleName[i], &(data[offset + 50 + 1]), kFileNameSize + 1);
                    }

                    // 5.5 LIBRARY CROSS-CHECK
                    // Match extracted references against generated libraries and mark missing assets.
                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                        // Instrument lookup in master index
                        if (instActive[i]) {
                            if ((f_open(&sd.file, "System/Data/Inst.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, 25) == FR_OK)) {
                                char searchName[kFileNameSize + 1];
                                bool searchActive = true;
                                uint16_t searchCounter = 0;
                                while (searchActive) {
                                    memset(searchName, 0x00, sizeof(searchName));
                                    sd_read(searchName, kFileNameSize);
                                    if (strcmp(instName[i], searchName) == 0) {
                                        instNum[i] = searchCounter;
                                        instMissing[i] = false;
                                        searchActive = false;
                                    } else if (searchCounter >= instLibrarySize) {
                                        instNum[i] = -1;
                                        instMissing[i] = true;
                                        searchActive = false;
                                    }
                                    searchCounter += 1;
                                }
                            } else {
                                instMissing[i] = true;
                            }
                            f_close(&sd.file);
                        } else {
                            instNum[i] = -1;
                            instMissing[i] = false;
                        }

                        // Apply instrument selection state to layer
                        if (instMissing[i]) {
                            layerInst_setInstSelected(i, -1);
                            layerInst_setInstLoaded(i);
                            layerInst_setSampleSelected(i, -1);
                            layerInst_setSampleLoaded(i);
                        } else {
                            layerInst_setInstSelected(i, instNum[i]);
                            layerInst_setInstLoaded(i);
                        }

                        // Sample lookup in per-instrument index
                        if ((sampleActive[i]) && (instActive[i]) && (!instMissing[i])) {
                            char instFileName[kFileNameSize + 15] = "System/Data/";
                            strncat(instFileName, instName[i], kFileNameSize);
                            strncat(instFileName, ".ins", 4);
                            if ((f_open(&sd.file, instFileName, FA_READ) == FR_OK) && (f_lseek(&sd.file, 25) == FR_OK)) {
                                char searchName[kFileNameSize + 1];
                                bool searchActive = true;
                                uint16_t searchCounter = 0;
                                while (searchActive) {
                                    memset(searchName, 0x00, kFileNameSize);
                                    sd_read(searchName, kFileNameSize);
                                    if (strcmp(sampleName[i], searchName) == 0) {
                                        sampleNum[i] = searchCounter;
                                        sampleMissing[i] = false;
                                        searchActive = false;
                                    } else if (searchCounter >= sampleLibrarySize[instNum[i]]) {
                                        sampleNum[i] = -1;
                                        sampleMissing[i] = true;
                                        searchActive = false;
                                    }
                                    searchCounter += 1;
                                }
                            } else {
                                sampleMissing[i] = true;
                            }
                            f_close(&sd.file);
                        } else {
                            sampleNum[i] = -1;
                            sampleMissing[i] = false;
                        }

                        // Apply sample selection state to layer
                        if (sampleMissing[i]) {
                            layerInst_setInstSelected(i, -1);
                            layerInst_setInstLoaded(i);
                            layerInst_setSampleSelected(i, -1);
                            layerInst_setSampleLoaded(i);
                        } else {
                            layerInst_setSampleSelected(i, sampleNum[i]);
                            layerInst_setSampleLoaded(i);
                        }
                    }

                    // 5.6 POST-LOAD VALIDATION
                    // Raise alert if any instrument/sample reference cannot be resolved.
                    uint8_t missingSample = 0;
                    for (uint8_t j = 0; j < kLayerLibrarySize; j++) {
                        if (instMissing[j]) missingSample += 1;
                        if (sampleMissing[j]) missingSample += 1;
                    }
                    if (missingSample) {
                        (missingSample == 1) ? alertType = ALERT_MISSINGSAMPLE : alertType = ALERT_MISSINGSAMPLES;
                        lcd_drawAlert();
                        HAL_Delay(1000);
                        result = SD_ERROR;
                    } else {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }

    // 6. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return result;
}

// System/Preset/SynthUser.rws: "RWSYNUS2" + 16 x [used, 21 values of synth::pack] + "EOF".
// ("RWSYNUSR" from a v1.13 test build has 20 values per slot and is still read.) Written whole on
// every save (363 bytes). No file = all USER slots empty.

void Controller::sd_loadSynthUser() {
    for (uint8_t i = 0; i < synth::kUserPresetCount; i++) {
        synthUserUsed[i] = false;
        synthUserPresets[i] = synth::Params{};
    }
    if (f_open(&sd.file, kSynthUserFile, FA_READ) != FR_OK) return;
    char ref[sizeof(kSynthUserRef)] = {};
    uint8_t data[synth::kUserPresetCount * (1 + synth::kPackedSize)];
    uint8_t values = 0;  // per slot; "RWSYNUSR" (test build) had 20
    if (sd_read(ref, strlen(kSynthUserRef)) == FR_OK) {
        if (strncmp(ref, kSynthUserRef, strlen(kSynthUserRef)) == 0) values = synth::kPackedSize;
        if (strncmp(ref, "RWSYNUSR", 8) == 0) values = 20;
    }
    const bool ok = (values > 0) && (sd_read(data, synth::kUserPresetCount * (1 + values)) == FR_OK);
    // v2.05: the MOD page of every slot, after the "EOF" (older files end there).
    char eof[3];
    char tag[sizeof(kSynthExtRef)] = {};
    uint8_t ext[synth::kUserPresetCount * synth::kExtSize];
    const bool extOk = ok && (values == synth::kPackedSize) && (sd_read(eof, 3) == FR_OK) && (sd_read(tag, strlen(kSynthExtRef)) == FR_OK) &&
                       (strncmp(tag, kSynthExtRef, strlen(kSynthExtRef)) == 0) && (sd_read(ext, sizeof(ext)) == FR_OK);
    f_close(&sd.file);
    if (!ok) return;
    for (uint8_t i = 0; i < synth::kUserPresetCount; i++) {
        const uint8_t* d = &data[i * (1 + values)];
        synthUserUsed[i] = (d[0] == 1);
        if (synthUserUsed[i]) synthUserPresets[i] = synth::unpack(d + 1, values);  // sanitized
        if (synthUserUsed[i] && extOk) synthUserPresets[i] = synth::unpackExt(synthUserPresets[i], &ext[i * synth::kExtSize]);
    }
}

SdResult Controller::sd_saveSynthUser() {
    uint8_t data[synth::kUserPresetCount * (1 + synth::kPackedSize)];
    uint8_t ext[synth::kUserPresetCount * synth::kExtSize];  // v2.05 MOD page, after the "EOF"
    for (uint8_t i = 0; i < synth::kUserPresetCount; i++) {
        uint8_t* d = &data[i * (1 + synth::kPackedSize)];
        d[0] = synthUserUsed[i] ? 1 : 0;
        synth::pack(synthUserPresets[i], d + 1);
        synth::packExt(synthUserPresets[i], &ext[i * synth::kExtSize]);
    }
    keyboard_disable();  // as for the other SD writes: no I2C keyboard traffic meanwhile
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) HAL_SD_Abort(&hsd2);
    SdResult result = SD_ERROR;
    if (f_open(&sd.file, kSynthUserFile, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        if ((sd_write(kSynthUserRef, strlen(kSynthUserRef)) == FR_OK) && (sd_write(data, sizeof(data)) == FR_OK) && (sd_write("EOF", 3) == FR_OK) &&
            (sd_write(kSynthExtRef, strlen(kSynthExtRef)) == FR_OK) && (sd_write(ext, sizeof(ext)) == FR_OK) && (sd_write("EOF", 3) == FR_OK)) {
            result = SD_OK;
        }
        if (f_close(&sd.file) != FR_OK) result = SD_ERROR;
    }
    keyboard_enable();
    return result;
}

void Controller::sd_loadDrumkitSynth(const char* drumkitName_) {
    /// @brief Reads the "SYK1" tail of a drumkit (MODE + SYNTH sound per layer, v1.13+). A kit
    /// saved by older firmware has none: its layers keep their current MODE and sound, so loading
    /// an old drum kit never wipes a synth line. Values are range-checked (sanitized).

    if (f_open(&sd.file, drumkitName_, FA_READ) != FR_OK) return;
    char tag[sizeof(kDrumkitSynthRef)] = {};
    uint8_t tail[kLayerLibrarySize * (1 + synth::kPackedSize)];
    uint8_t perLayer = 0;  // bytes per layer: MODE + sound values
    uint8_t used = 0;      // sound values to take
    if ((f_lseek(&sd.file, strlen(kDrumkitRef) + kDrumkitByteSize + 3) == FR_OK) &&  // + "EOF"
        (sd_read(tag, strlen(kDrumkitSynthRef)) == FR_OK)) {
        if (strncmp(tag, "SYK4", 4) == 0) {
            perLayer = 1 + synth::kPackedSize;
            used = synth::kPackedSize;
        } else if (strncmp(tag, "SYK3", 4) == 0) {  // v1.13 test build: 20 values (no PAD HOLD)
            perLayer = 21;
            used = 20;
        } else if (strncmp(tag, "SYK2", 4) == 0) {  // v1.13 test build: 15 values (no AMP page yet)
            perLayer = 16;
            used = 15;
        } else if (strncmp(tag, "SYK1", 4) == 0) {  // v1.13 test build: 11 values, the 11th is ignored
            perLayer = 12;
            used = 10;
        }
    }
    const bool ok = (perLayer > 0) && (sd_read(tail, kLayerLibrarySize * perLayer) == FR_OK);
    // v2.05: the MOD page of every layer, after the tail's "EOF" (older kits end there).
    char eof[3];
    char extTag[sizeof(kSynthExtRef)] = {};
    uint8_t ext[kLayerLibrarySize * synth::kExtSize];
    const bool extOk = ok && (used == synth::kPackedSize) && (sd_read(eof, 3) == FR_OK) && (sd_read(extTag, strlen(kSynthExtRef)) == FR_OK) &&
                       (strncmp(extTag, kSynthExtRef, strlen(kSynthExtRef)) == 0) && (sd_read(ext, sizeof(ext)) == FR_OK);
    f_close(&sd.file);
    if (!ok) return;

    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        const uint8_t* d = &tail[i * perLayer];
        if (layerLibrary[i].toneMode == tone::kModeSynth) synth_silence(i);  // the sound changes under it
        layerLibrary[i].synth = synth::unpack(d + 1, used);
        if (extOk) layerLibrary[i].synth = synth::unpackExt(layerLibrary[i].synth, &ext[i * synth::kExtSize]);
        synthPresetIndex[i] = -1;  // the name comes from the sound itself now
        tone_setMode(i, (d[0] < tone::kModeCount) ? d[0] : tone::kModeDrum);
        lcd_drawLayerTab_InstData(i);
    }
}

SdResult Controller::sd_saveDrumkit(uint8_t kitNum_) {
    /// @brief Serializes current drumkit state and writes it to the selected SD slot.
    /// Encodes processor parameters, layer settings, asset references, and final EOF marker.

    SdResult result = SD_ERROR;
    char eof[] = "EOF";
    char drumkitName[50];
    char drumkitNum[4];

    // 1. FILENAME CONSTRUCTION
    // Generate the 3-digit padded filename (e.g., Drumkit_001.rws).
    sprintf(drumkitNum, "%03d", (kitNum_ + 1));
    strcpy(drumkitName, kDrumkitStart);
    strcat(drumkitName, drumkitNum);
    strcat(drumkitName, kDrumkitEnd);

    // 2. PAYLOAD ENCODING
    // Build the drumkit payload buffer before starting SD write operations.

    // 2.1 SLOT MARKER
    // Mark file as active drumkit slot.
    uint8_t data[kDrumkitByteSize] = {0x00};
    data[0] = '*';

    // 2.2 MIXER SETTINGS
    data[1] = mixer.masterVolume;
    data[2] = mixer.hpVolume;
    data[3] = mixer.loVolume;
    data[4] = mixer.liVolume;
    data[5] = mixer.liMixerIn;
    data[6] = mixer.pan;
    data[7] = mixer.limiter;
    data[8] = mixer.normalize;
    data[9] = mixer.masterMute;
    data[10] = mixer.hpMute;
    data[11] = mixer.loMute;
    data[12] = mixer.liMute;

    // 2.3 RHYTHM SETTINGS
    data[13] = rhythm.tempo;
    data[14] = rhythm.measure;
    data[15] = rhythm.bar;
    data[16] = rhythm.quantize;

    // 2.4 METRONOME SETTINGS
    data[17] = metronome.active;
    data[18] = metronome.precount;
    data[19] = metronome.sample;
    data[20] = metronome.volume;

    // 2.5 I/O SYNC SETTINGS
    data[21] = io.syncIn;
    data[22] = io.syncOut;
    data[23] = (uint8_t)((io.midiIn << 4) | io.midiInChannel);
    data[24] = (uint8_t)((io.midiOut << 4) | io.midiOutChannel);

    // 2.6 EQUALIZER SETTINGS
    data[25] = eq.active;
    data[26] = eq.q;
    data[27] = eq.freqLowShelf;
    data[28] = eq.gainLowShelf;
    data[29] = eq.freqHighShelf;
    data[30] = eq.gainHighShelf;
    data[31] = eq.freqPeak[0];
    data[32] = eq.freqPeak[1];
    data[33] = eq.freqPeak[2];
    data[34] = eq.freqPeak[3];
    data[35] = eq.gainPeak[0];
    data[36] = eq.gainPeak[1];
    data[37] = eq.gainPeak[2];
    data[38] = eq.gainPeak[3];

    // 2.7 FILTER SETTINGS (0 AND 1)
    data[39] = filter[0].active;
    data[40] = filter[0].type;
    data[41] = filter[0].frequency;
    data[42] = filter[0].resonance;
    data[43] = filter[0].slope;
    data[44] = filter[0].drive;
    data[45] = filter[0].dry;
    data[46] = filter[0].wet;

    data[47] = filter[1].active;
    data[48] = filter[1].type;
    data[49] = filter[1].frequency;
    data[50] = filter[1].resonance;
    data[51] = filter[1].slope;
    data[52] = filter[1].drive;
    data[53] = filter[1].dry;
    data[54] = filter[1].wet;

    // 2.8 EFFECT SETTINGS (A/B + SUBEFFECTS)
    data[55] = effect[0].active;
    data[56] = effect[0].type;
    data[57] = effect[1].active;
    data[58] = effect[1].type;
    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
            uint16_t baseNum = 59 + (54 * i) + (6 * j);
            data[baseNum + 0] = effect[i].subEffect[j].aData;
            data[baseNum + 1] = effect[i].subEffect[j].bData;
            data[baseNum + 2] = effect[i].subEffect[j].cData;
            data[baseNum + 3] = effect[i].subEffect[j].dData;
            data[baseNum + 4] = effect[i].subEffect[j].eData;
            data[baseNum + 5] = effect[i].subEffect[j].fData;
        }
    }

    // 2.9 REVERB SETTINGS
    data[167] = reverb.active;
    data[168] = reverb.size;
    data[169] = reverb.decay;
    data[170] = reverb.damping;
    data[171] = reverb.preDelay;
    data[172] = reverb.surround;
    data[173] = reverb.dry;
    data[174] = reverb.wet;

    // 2.10 LFO SETTINGS
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        Lfo& lfo = lfoLibrary[i];
        uint16_t offset = 180 + (i * 6);
        data[offset + 0] = lfo.active;
        data[offset + 1] = lfo.type;
        data[offset + 2] = lfo.rate;
        data[offset + 3] = lfo.phase;
        data[offset + 4] = lfo.invert;
        data[offset + 5] = lfo.depth;
    }

    // 2.11 LAYER SETTINGS
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        Layer& layer = layerLibrary[i];
        uint16_t offset = 240 + (i * 25);
        data[offset + 0] = layer.level;
        data[offset + 1] = layer.pitch;
        data[offset + 2] = layer.direction;
        data[offset + 3] = layer.resolution;
        data[offset + 4] = layer.start;
        data[offset + 5] = layer.end;
        data[offset + 6] = layer.timing;
        data[offset + 7] = layer.probability;
        data[offset + 8] = layer.lfoLevel;
        data[offset + 9] = layer.lfoPitch;
        data[offset + 10] = layer.lfoDirection;
        data[offset + 11] = layer.lfoResolution;
        data[offset + 12] = layer.lfoStart;
        data[offset + 13] = layer.lfoEnd;
        data[offset + 14] = layer.lfoTiming;
        data[offset + 15] = layer.lfoProbability;
        data[offset + 16] = layer.eq;
        data[offset + 17] = layer.filter;
        data[offset + 18] = layer.effect;
        data[offset + 19] = layer.reverb;
        data[offset + 20] = layer.mute;
        data[offset + 21] = layer.fill;
        data[offset + 22] = layer.style;
    }

    // 2.12 ASSET REFERENCES
    // Store per-layer instrument/sample activity flags and names.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        Layer& layer = layerLibrary[i];
        uint16_t offset = 500 + (i * 100);
        if (layer.instLoaded != -1) {
            data[offset] = 'x';
            for (uint8_t j = 0; j < kFileNameSize; j++) {
                data[offset + 1 + j] = layer.instLoadedData.nameLong[j];
            }
        } else {
            data[offset] = 'o';
        }
        if (layer.sampleLoaded != -1) {
            data[offset + 50] = 'x';
            for (uint8_t j = 0; j < kFileNameSize; j++) {
                data[offset + 50 + 1 + j] = layer.sampleLoadedData.nameLong[j];
            }
        } else {
            data[offset + 50] = 'o';
        }
    }

    // v2.02: the drumkit's name.
    pname::encode(kitName, &data[pname::kNameOffset]);

    // 3. INPUT PROTECTION
    // Disable keyboard interrupts during SD write operations.
    keyboard_disable();

    // 4. SD STATE RECOVERY
    // Close stale handles and abort unfinished SD transactions.
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
        HAL_SD_Abort(&hsd2);
    }

    // 5. FILE WRITE PIPELINE (crash-safe since v2.01: temporary file + commit)
    // Write header, drumkit block, and EOF marker.
    sd_beginSafeSave();
    if (sd_checkDrumkit(kitNum_) == SD_OK) {
        if (f_open(&sd.file, kSaveTempMain, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            if (sd_write(kDrumkitRef, strlen(kDrumkitRef)) == FR_OK) {
                // 5.1 DRUMKIT BLOCK WRITE
                // Stream encoded drumkit payload in 512-byte chunks.
                const uint16_t writeByteSize = 512;
                uint16_t writeCountTotal = (kDrumkitByteSize / writeByteSize);
                uint16_t remainderByteSize = (kDrumkitByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;
                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (sd_write((data + pointerOffset), writeByteSize) != FR_OK) errorCount += 1;
                    pointerOffset += 512;
                }

                // 5.2 DRUMKIT TAIL WRITE
                // Write final non-512-aligned remainder.
                if (remainderByteSize) {
                    if (sd_write((data + pointerOffset), remainderByteSize) != FR_OK) errorCount += 1;
                }

                // 5.3 EOF COMMIT
                // Commit EOF marker only when all previous writes succeeded.
                if (errorCount == 0) {
                    if (sd_write(eof, strlen(eof)) == FR_OK) {
                        // 5.4 SYNTH TAIL (v1.13): MODE + SYNTH sound of every layer after the
                        // "EOF". Older firmware reads a fixed size and never sees it.
                        uint8_t tail[kLayerLibrarySize * (1 + synth::kPackedSize)];
                        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                            uint8_t* d = &tail[i * (1 + synth::kPackedSize)];
                            d[0] = layerLibrary[i].toneMode;
                            synth::pack(layerLibrary[i].synth, d + 1);
                        }
                        // v2.05: the synth MOD page after it (older firmware stops before).
                        uint8_t ext[kLayerLibrarySize * synth::kExtSize];
                        for (uint8_t i = 0; i < kLayerLibrarySize; i++) synth::packExt(layerLibrary[i].synth, &ext[i * synth::kExtSize]);
                        if ((sd_write(kDrumkitSynthRef, strlen(kDrumkitSynthRef)) == FR_OK) && (sd_write(tail, sizeof(tail)) == FR_OK) &&
                            (sd_write(eof, strlen(eof)) == FR_OK) && (sd_write(kSynthExtRef, strlen(kSynthExtRef)) == FR_OK) &&
                            (sd_write(ext, sizeof(ext)) == FR_OK) && (sd_write(eof, strlen(eof)) == FR_OK)) {
                            result = SD_OK;
                        }
                    }
                }
            }
        }
        // f_close flushes the last cached sector; a failure here means the file is incomplete.
        if (f_close(&sd.file) != FR_OK) result = SD_ERROR;
        if (result == SD_OK) result = sd_commitSafeSave(drumkitName, "", autosave::kSideKeep);
    }
    if (result != SD_OK) sd_beginSafeSave();

    // 6. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return result;
}

SdResult Controller::sd_clearDrumkit(uint8_t kitNum_) {
    /// @brief Overwrites a drumkit preset slot with default factory parameters.
    /// Resets all instrument settings and FX chains to a default template.

    SdResult result = SD_ERROR;
    char eof[] = "EOF";
    char drumkitName[50];
    char drumkitNum[4];

    // 1. FILENAME CONSTRUCTION
    // Build the target path (e.g., System/Drumkit/Drumkit_001.rws).
    sprintf(drumkitNum, "%03d", (kitNum_ + 1));
    strcpy(drumkitName, kDrumkitStart);
    strcat(drumkitName, drumkitNum);
    strcat(drumkitName, kDrumkitEnd);

    // 2. INPUT PROTECTION + SD STATE RECOVERY
    // Lock keyboard input and ensure SDMMC is in a clean 'Ready' state.
    keyboard_disable();
    f_close(&sd.file);
    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
        HAL_SD_Abort(&hsd2);
    }

    // 3. FILE RE-CREATION (crash-safe since v2.01: temporary file + commit)
    // Re-create the target slot to wipe any previous drumkit data.
    sd_beginSafeSave();
    if (sd_checkDrumkit(kitNum_) == SD_OK) {
        if (f_open(&sd.file, kSaveTempMain, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            // 4. HEADER WRITE
            // Write standard drumkit signature header.
            if (sd_write(kDrumkitRef, strlen(kDrumkitRef)) == FR_OK) {
                const uint16_t writeByteSize = 512;  // Block-aligned size for SD efficiency
                const uint16_t drumkitByteSize = kDrumkitByteSize;
                uint16_t writeCountTotal = (drumkitByteSize / writeByteSize);
                uint16_t remainderByteSize = (drumkitByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                // 5. DEFAULT PAYLOAD WRITE
                // Stream default drumkit template in fixed-size blocks.

                // 5.1 DRUMKIT BLOCK WRITE
                // Write default drumkit data in 512-byte chunks.
                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (sd_write((kDataDrumkit + pointerOffset), writeByteSize) != FR_OK) errorCount += 1;
                    pointerOffset += 512;
                }

                // 5.2 DRUMKIT TAIL WRITE
                // Write final non-512-aligned drumkit remainder.
                if (remainderByteSize) {
                    if (sd_write((kDataDrumkit + pointerOffset), remainderByteSize) != FR_OK) errorCount += 1;
                }

                // 5.3 EOF COMMIT
                // Seal the file with EOF only if all previous writes succeeded.
                if (errorCount == 0) {
                    if (sd_write(eof, strlen(eof)) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        // f_close flushes the last cached sector; a failure here means the file is incomplete.
        if (f_close(&sd.file) != FR_OK) result = SD_ERROR;
        if (result == SD_OK) result = sd_commitSafeSave(drumkitName, "", autosave::kSideKeep);
    }
    if (result != SD_OK) sd_beginSafeSave();

    // 6. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return result;
}

SdResult Controller::sd_checkSamplesInUse() {
    /// @brief Cross-checks active instrument and sample selections against SD index files.
    /// Clears invalid references and reports missing assets to the user.

    uint8_t missingSampleCount = 0;
    bool instMissing[kLayerLibrarySize] = {false};
    bool sampleMissing[kLayerLibrarySize] = {false};
    char buffer[kFileNameSize];
    char pathBuffer[64];

    // 1. INPUT PROTECTION
    // Lock keyboard interrupts to ensure exclusive access to the SDMMC bus.
    keyboard_disable();

    // 2. INSTRUMENT INDEX VALIDATION
    // Verify loaded instrument references against the master instrument index.
    if (f_open(&sd.file, "System/Data/Inst.lib", FA_READ) == FR_OK) {
        const uint32_t headerOffset = 25;
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            Layer& layer = layerLibrary[i];

            // Skip layers without an assigned instrument
            if (layer.instLoaded == -1) continue;

            // Seek exact record address in index file
            FSIZE_t targetAddr = headerOffset + (layer.instLoaded * kFileNameSize);
            if (f_lseek(&sd.file, targetAddr) == FR_OK) {
                UINT bytesRead;
                if (f_read(&sd.file, buffer, kFileNameSize, &bytesRead) == FR_OK) {
                    buffer[kFileNameSize - 1] = 0;

                    // Mark missing if indexed name does not match loaded name
                    if (strncmp(layer.instLoadedData.nameLong, buffer, kFileNameSize) != 0) {
                        instMissing[i] = true;
                    }
                } else {
                    instMissing[i] = true;
                }
            } else {
                instMissing[i] = true;
            }

            // 3. INSTRUMENT STATE RECOVERY
            // Reset instrument and dependent sample state when instrument is missing.
            if (instMissing[i]) {
                layerInst_setInstSelected(i, -1);
                layerInst_setInstLoaded(i);
                layerInst_setSampleSelected(i, -1);
                layerInst_setSampleLoaded(i);
                missingSampleCount++;
            }
        }
        f_close(&sd.file);
    } else {
        // Master index missing: invalidate all currently loaded instruments.
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            if (layerLibrary[i].instLoaded != -1) {
                instMissing[i] = true;
                missingSampleCount++;
            }
        }
    }

    // 4. SAMPLE INDEX VALIDATION
    // Verify loaded sample references inside per-instrument sample index files.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        Layer& layer = layerLibrary[i];
        if (instMissing[i] || layer.instLoaded == -1 || layer.sampleLoaded == -1) continue;

        // Open instrument-specific sample index (.ins)
        strcpy(pathBuffer, "System/Data/");
        strncat(pathBuffer, layer.instLoadedData.nameLong, kFileNameSize);
        strcat(pathBuffer, ".ins");

        if (f_open(&sd.file, pathBuffer, FA_READ) == FR_OK) {
            const uint32_t headerOffset = 25;
            FSIZE_t targetAddr = headerOffset + (layer.sampleLoaded * kFileNameSize);
            if (f_lseek(&sd.file, targetAddr) == FR_OK) {
                UINT bytesRead;
                if (f_read(&sd.file, buffer, kFileNameSize, &bytesRead) == FR_OK) {
                    buffer[kFileNameSize - 1] = 0;
                    if (strncmp(layer.sampleLoadedData.nameLong, buffer, kFileNameSize) != 0) {
                        sampleMissing[i] = true;
                    }
                } else {
                    sampleMissing[i] = true;
                }
            } else {
                sampleMissing[i] = true;
            }
            f_close(&sd.file);
        } else {
            sampleMissing[i] = true;
        }

        // 5. SAMPLE STATE RECOVERY
        // Reset sample state when the referenced sample is missing.
        if (sampleMissing[i]) {
            layerInst_setSampleSelected(i, -1);
            layerInst_setSampleLoaded(i);
            missingSampleCount++;
        }
    }

    // 6. POST-VALIDATION ALERT
    // Notify user when one or more referenced assets are missing.
    if (missingSampleCount > 0) {
        alertType = (missingSampleCount == 1) ? ALERT_MISSINGSAMPLE : ALERT_MISSINGSAMPLES;
        lcd_drawAlert();
        HAL_Delay(1000);  // Give user time to read the alert
        lcd_clearAlert();
        keyboard_enable();
        return SD_ERROR;
    }

    // 7. INPUT RESTORATION
    // Re-enable keyboard interrupts before returning to caller.
    keyboard_enable();
    return SD_OK;
}

FRESULT Controller::sd_createDirectory(const char* path) {
    /// @brief Creates a directory at the specified filesystem path.
    /// Thin wrapper around FatFS `f_mkdir` for SD storage operations.

    // 1. DIRECTORY CREATION
    // Attempt to create the folder and return the FatFS result code.
    return f_mkdir(path);
}

FRESULT Controller::sd_deleteDirectory(const char* path) {
    /// @brief Recursively deletes a directory and all nested entries.
    /// Uses depth-first traversal so folders are removed only after contents are cleared.

    FRESULT result;
    DIR dir;
    FILINFO fileInfo;
    char pathBuffer[256];  // Buffer to construct the full path for recursion

    // 1. DIRECTORY ACCESS
    // Attempt to open the target directory. Exit if it doesn't exist or is inaccessible.
    result = f_opendir(&dir, path);
    if (result != FR_OK) return result;

    // 2. ENTRY ITERATION
    // Iterate through every entry (file or folder) in the current directory.
    while (true) {
        result = f_readdir(&dir, &fileInfo);

        // Exit on read error or end-of-directory marker.
        if (result != FR_OK || fileInfo.fname[0] == 0) {
            break;
        }

        // 3. SPECIAL ENTRY FILTERING
        // Skip '.' and '..' to avoid self/parent recursion.
        if (fileInfo.fname[0] == '.') continue;

        // 4. PATH CONSTRUCTION
        // Build full child path from parent path + entry name.
        // A long file name (up to 255 chars) plus the parent path can exceed the buffer;
        // refuse rather than overflow the stack.
        int pathLen = snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s", path, fileInfo.fname);
        if ((pathLen < 0) || ((size_t)pathLen >= sizeof(pathBuffer))) {
            f_closedir(&dir);
            return FR_INVALID_NAME;
        }

        // 5. TYPE-SPECIFIC DELETION
        if (fileInfo.fattrib & AM_DIR) {
            // 5.1 SUBDIRECTORY DELETE
            // Recursively clear and remove child directory.
            result = sd_deleteDirectory(pathBuffer);
        } else {
            // 5.2 FILE DELETE
            // Remove file entry directly.
            result = f_unlink(pathBuffer);
        }

        // 6. ERROR PROPAGATION
        // Stop immediately if any delete operation fails.
        if (result != FR_OK) {
            f_closedir(&dir);
            return result;
        }
    }

    // 7. RESOURCE CLEANUP
    // Close the directory handle before attempting to delete the folder itself.
    f_closedir(&dir);

    // 8. PARENT FOLDER REMOVAL
    // Remove parent directory after all nested entries are deleted.
    return f_unlink(path);
}

void Controller::sd_debug(const char* msg, FRESULT res) {
    /// @brief Renders SD/FatFS error diagnostics on the LCD.
    /// Used for quick runtime visibility during development and failure handling.

    // 1. UI STYLING
    // Configure high-contrast text style for readability.
    lcd.setForeColor(kAccent2Color);
    lcd.setBackColor(BLACK);
    lcd.setAlignment(LEFT);
    lcd.setFont(FONT_07x09);

    // 2. ERROR STRING FORMATTING
    // Compose message + numeric FRESULT code into a single debug string.
    char buf[32];
    snprintf(buf, sizeof(buf), "SD ERR: %s (%d)", msg, (int)res);

    // 3. LCD OUTPUT
    // Draw the debug string at a fixed on-screen position.
    lcd.drawText(buf, strlen(buf), 100, 100);
}

FRESULT Controller::sd_write(const void* buffer, UINT size) {
    /// @brief Writes `size` bytes to sd.file and treats a short write as failure.
    /// FatFs returns FR_OK with fewer bytes written when the card is full, which would
    /// otherwise leave a silently truncated file behind.

    FRESULT result = f_write(&sd.file, buffer, size, &sd.byteswritten);
    if ((result == FR_OK) && (sd.byteswritten != size)) {
        result = FR_DENIED;  // FatFs convention for "volume full"
    }
    return result;
}

FRESULT Controller::sd_read(void* buffer, UINT size) {
    /// @brief Reads `size` bytes from sd.file and treats a short read as failure.
    /// Used for fixed-size headers and records, where hitting EOF early means the file is
    /// truncated or corrupt and the buffer must not be trusted.

    FRESULT result = f_read(&sd.file, buffer, size, &sd.bytesread);
    if ((result == FR_OK) && (sd.bytesread != size)) {
        result = FR_INT_ERR;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
/* Sdram Functions -----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::sdram_fadeOutAudio(uint32_t ramAddress_, uint32_t sampleSize_, uint16_t fadeOutSize_) {
    /// @brief Applies a linear fade-out envelope directly to audio data stored in SDRAM.
    /// Used to prevent DC offset clicks at the end of sampled loops or recorded takes.

    // 1. BOUNDARY PROTECTION
    // Ensure the fade-out length does not exceed the total sample length
    if (fadeOutSize_ > sampleSize_) {
        fadeOutSize_ = (uint16_t)sampleSize_;
    }

    // 2. ADDRESS CALCULATION
    // Offset calculation: (Total samples - Fade window) * 4 bytes (size of float)
    uint32_t startOffset = (sampleSize_ - (uint32_t)fadeOutSize_) * 4;
    uint32_t currentAddress = ramAddress_ + startOffset;

    // 3. ENVELOPE INITIALIZATION
    // Calculate the slope (decrement) per sample to reach zero exactly at the end
    float decrement = 1.0f / (float)fadeOutSize_;
    float multiplier = 1.0f;

    // Pointer to the start of the fade-out region in SDRAM
    volatile float* ptr = (volatile float*)currentAddress;

    // 4. PROCESSING LOOP
    // Iteratively multiply each 32-bit float sample by the decreasing gain multiplier
    for (uint16_t i = 0; i < fadeOutSize_; i++) {
        // Read, modify, and write back to SDRAM
        float val = *ptr;
        val *= multiplier;
        *ptr = val;

        // Move to the next 4-byte float address
        ptr++;

        // Update linear envelope gain
        multiplier -= decrement;

        // Floor at zero to prevent negative gain artifacts due to precision drift
        if (multiplier < 0.0f) {
            multiplier = 0.0f;
        }
    }
}

