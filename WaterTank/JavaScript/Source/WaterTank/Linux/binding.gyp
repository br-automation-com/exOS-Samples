{
  "targets": [
    {
      "target_name": "l_WaterTank",
      "sources": [
        "libwatertank.c",
        "exos_watertank.c"
      ],
      "include_dirs": [
        '/usr/include'
      ],  
      'link_settings': {
        'libraries': [
          '-lexos-api',
          '-lzmq'
        ]
      }
    }
  ]
}
