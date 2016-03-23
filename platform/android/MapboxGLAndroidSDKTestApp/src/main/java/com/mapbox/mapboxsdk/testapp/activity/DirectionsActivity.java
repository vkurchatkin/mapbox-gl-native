package com.mapbox.mapboxsdk.testapp.activity;

import android.app.FragmentTransaction;
import android.graphics.Color;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import com.mapbox.directions.DirectionsCriteria;
import com.mapbox.directions.MapboxDirections;
import com.mapbox.directions.service.models.DirectionsResponse;
import com.mapbox.directions.service.models.DirectionsRoute;
import com.mapbox.directions.service.models.Waypoint;
import com.mapbox.mapboxsdk.annotations.MarkerOptions;
import com.mapbox.mapboxsdk.annotations.PolylineOptions;
import com.mapbox.mapboxsdk.camera.CameraPosition;
import com.mapbox.mapboxsdk.camera.CameraUpdateFactory;
import com.mapbox.mapboxsdk.constants.Style;
import com.mapbox.mapboxsdk.geometry.LatLng;
import com.mapbox.mapboxsdk.maps.MapFragment;
import com.mapbox.mapboxsdk.maps.MapboxMap;
import com.mapbox.mapboxsdk.maps.MapboxMapOptions;
import com.mapbox.mapboxsdk.maps.OnMapReadyCallback;
import com.mapbox.mapboxsdk.testapp.R;
import com.mapbox.mapboxsdk.testapp.utils.ApiAccess;
import com.mapbox.mapboxsdk.maps.MapView;

import java.util.List;

import retrofit.Callback;
import retrofit.Response;
import retrofit.Retrofit;

public class DirectionsActivity extends AppCompatActivity implements OnMapReadyCallback {

    private final static String LOG_TAG = "DirectionsActivity";

    // Dupont Circle (Washington, DC)
    private final static Waypoint WAYPOINT_ORIGIN = new Waypoint(-77.04341, 38.90962);

    // The White House (Washington, DC)
    private final static Waypoint WAYPOINT_DESTINATION = new Waypoint(-77.0365, 38.8977);

    private MapboxMap mapboxMap;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_directions);

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowHomeEnabled(true);
        }

        MapFragment mapFragment;
        if (savedInstanceState == null) {
            FragmentTransaction transaction = getFragmentManager().beginTransaction();

            MapboxMapOptions options = new MapboxMapOptions();
            options.styleUrl(Style.EMERALD);
            options.accessToken(getString(R.string.mapbox_access_token));
            options.compassEnabled(false);
            options.attributionEnabled(false);
            options.logoEnabled(false);

            // Calculate center camera target
            LatLng centroid = new LatLng(
                    (WAYPOINT_ORIGIN.getLatitude() + WAYPOINT_DESTINATION.getLatitude()) / 2,
                    (WAYPOINT_ORIGIN.getLongitude() + WAYPOINT_DESTINATION.getLongitude()) / 2);

            // Inital camera position
            options.camera(new CameraPosition.Builder()
                    .target(centroid)
                    .zoom(14)
                    .build());

//            options.minZoom(1);
//            options.maxZoom(18);

            mapFragment = MapFragment.newInstance(options);

            transaction.add(R.id.fragmentContainer, mapFragment, LOG_TAG);
            transaction.commit();
        } else {
            mapFragment = (MapFragment) getFragmentManager().findFragmentByTag(LOG_TAG);
        }

        mapFragment.getMapAsync(this);
    }

    @Override
    public void onMapReady(@NonNull MapboxMap mapboxMap) {
        this.mapboxMap = mapboxMap;
        loadRoute();
    }

    private void loadRoute() {
        // Add origin and destination to the map
        mapboxMap.addMarker(new MarkerOptions()
                .position(new LatLng(WAYPOINT_ORIGIN.getLatitude()
                        , WAYPOINT_ORIGIN.getLongitude()))
                .title("Origin")
                .snippet("Dupont Circle"));

        mapboxMap.addMarker(new MarkerOptions()
                .position(new LatLng(WAYPOINT_DESTINATION.getLatitude(),
                        WAYPOINT_DESTINATION.getLongitude()))
                .title("Destination")
                .snippet("The White House"));

        // Get route from API
        getRoute(WAYPOINT_ORIGIN, WAYPOINT_DESTINATION);
    }

    private void getRoute(Waypoint origin, Waypoint destination) {
        MapboxDirections md = new MapboxDirections.Builder()
                .setAccessToken(ApiAccess.getToken(this))
                .setOrigin(origin)
                .setDestination(destination)
                .setProfile(DirectionsCriteria.PROFILE_WALKING)
                .build();

        md.enqueue(new Callback<DirectionsResponse>() {
            @Override
            public void onResponse(Response<DirectionsResponse> response, Retrofit retrofit) {
                // You can get generic HTTP info about the response
                Log.d(LOG_TAG, "Response code: " + response.code());

                // Print some info about the route
                DirectionsRoute currentRoute = response.body().getRoutes().get(0);
                Log.d(LOG_TAG, "Distance: " + currentRoute.getDistance());

                // Draw the route on the map
                drawRoute(currentRoute);
            }

            @Override
            public void onFailure(Throwable t) {
                Log.e(LOG_TAG, "Error: " + t.getMessage());
            }
        });
    }

    private void drawRoute(DirectionsRoute route) {
        // Convert List<Waypoint> into LatLng[]
        List<Waypoint> waypoints = route.getGeometry().getWaypoints();
        LatLng[] point = new LatLng[waypoints.size()];
        for (int i = 0; i < waypoints.size(); i++) {
            point[i] = new LatLng(
                    waypoints.get(i).getLatitude(),
                    waypoints.get(i).getLongitude());
        }

        // Draw Points on MapView
        mapboxMap.addPolyline(new PolylineOptions()
                .add(point)
                .color(Color.parseColor("#3887be"))
                .width(5));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

}