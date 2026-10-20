# GW Merge Source Pipeline

This pipeline keeps the work split into explicit stages so you can inspect every intermediate artifact.

## Layout

- `01_fetch_catalog.py`
  Fetch the paginated GWOSC catalog event list and write `data/gwosc/catalog_events.json`.
- `02_fetch_event_metadata.py`
  Fetch per-event detail and parameter metadata, then write `data/gwosc/event_index.json`.
- `03_resolve_skymaps.py`
  Download missing skymap archives, resolve one representative sky coordinate per event, then write `data/gwosc/resolved_sources.json`.
- `04_export_csv.py`
  Export the resolved coordinates to `data/gwosc/merge_sources.csv` and a readable summary report.

## Commands

Run the steps from the repository root:

```bash
python3 gw-pipeline/01_fetch_catalog.py --lastver
python3 gw-pipeline/02_fetch_event_metadata.py
python3 gw-pipeline/03_resolve_skymaps.py
python3 gw-pipeline/04_export_csv.py
```

If your goal is "events that are likely to have PE skymaps", prefer explicit released catalogs instead of the cumulative newest `GWTC` feed:

```bash
python3 gw-pipeline/01_fetch_catalog.py \
  --release GWTC-1-confident,GWTC-2.1-confident,GWTC-3-confident,GWTC-4.0 \
  --lastver
python3 gw-pipeline/02_fetch_event_metadata.py
```

The cumulative `GWTC` catalog includes newer `GWTC-5.0` entries where many events currently expose only
search-pipeline metadata and no PE/skymap link yet.

For quick smoke tests on a small batch:

```bash
python3 gw-pipeline/02_fetch_event_metadata.py --limit 10
python3 gw-pipeline/03_resolve_skymaps.py --limit 10
```

Limited runs write to suffixed files such as `event_index.limit-10.json` and
`resolved_sources.limit-10.json` so they do not overwrite the full outputs.

## Optional Python dependencies

Steps 1, 2 and 4 only need `requests`, which is already present in this environment.

Step 3 also needs:

```bash
python3 -m pip install astropy astropy-healpix
```

## Output files

- `data/gwosc/catalog_events.json`
- `data/gwosc/event_index.json`
- `data/gwosc/resolved_sources.json`
- `data/gwosc/merge_sources.csv`
- `data/gwosc/merge_sources_report.md`

## Cache directories

- `data/gwosc/cache/details/`
- `data/gwosc/cache/parameters/`
- `data/gwosc/cache/skymaps/`
- `data/gwosc/cache/resolved/`

The pipeline reuses cached files by default, so incremental updates only touch new or refreshed events.
